// Samuli Pylkkönen
// TVT22SPL
// max pisteet

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys_clock.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "TimeParser.h"
#include "TrafficParser.h"

// LED-pinien määritykset
static const struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);   // Punainen
static const struct gpio_dt_spec green_led = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios); // Vihreä

// UART-initialisointi
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define STACK_SIZE 2048
#define PRIORITY 5
#define MAX_MSG_LEN 256 // Suurensin pituutta varmuuden vuoksi
#define DEFAULT_DURATION_MS 1000

K_MSGQ_DEFINE(uart_msgq, MAX_MSG_LEN, 10, 4);

// Määrittele säikeiden pinot ja ohjauslohkot
K_THREAD_STACK_DEFINE(uart_receive_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(dispatcher_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(red_task_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(yellow_task_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(green_task_stack, STACK_SIZE);

struct k_thread uart_receive_thread_data;
struct k_thread dispatcher_thread_data;
struct k_thread red_task_thread_data;
struct k_thread yellow_task_thread_data;
struct k_thread green_task_thread_data;
struct k_timer traffic_light_timer;

// Määrittele FIFO-puskurit
K_FIFO_DEFINE(red_fifo);
K_FIFO_DEFINE(yellow_fifo);
K_FIFO_DEFINE(green_fifo);

// Määrittele condition variablet ja muteksit
struct k_condvar red_condvar;
struct k_condvar yellow_condvar;
struct k_condvar green_condvar;
struct k_mutex led_mutex;            // Suojaa FIFO-puskureita ja condition variableja
struct k_mutex light_mutex;          // Varmistaa, että vain yksi valo on päällä kerrallaan
struct k_mutex total_duration_mutex; // Suojaa total_duration_us

// Määrittele komentojono
K_FIFO_DEFINE(command_queue);

struct command_item
{
    void *fifo_reserved; // Ensimmäinen kenttä varattu kernelille
    char color;
};

// Määrittele ring buffer
struct ring_buffer
{
    char buffer[MAX_MSG_LEN];
    int head;
    int tail;
    int count;
};

struct ring_buffer uart_buffer = {.head = 0, .tail = 0, .count = 0};
char uart_msg[MAX_MSG_LEN];
int uart_msg_index = 0;

// Määrittele led_item-rakenne
struct led_item
{
    void *fifo_reserved; // Ensimmäinen kenttä varattu kernelille
    int duration;
};

// Aikamuuttujat
static uint32_t total_duration_us = 0;
static uint32_t uart_sequence_start_time = 0;
static uint32_t uart_sequence_end_time = 0;
static uint32_t dispatcher_processing_time_us = 0;
static bool debug_enabled = false;

#define DEBUG_PRINT(fmt, ...)           \
    do                                  \
    {                                   \
        if (debug_enabled)              \
            printk(fmt, ##__VA_ARGS__); \
    } while (0)

// Sekvenssin suorituksen synkronointiin
struct k_sem sequence_sem;

// Forward declarations
void set_traffic_light_timer(int seconds);

// UART-initialisointifunktio
int init_uart(void)
{
    if (!device_is_ready(uart_dev))
    {
        printk("UART device not ready!\n");
        return 1;
    }
    printk("UART initialized successfully\n");
    return 0;
}

// GPIO-initialisointifunktio
int init_gpio(void)
{
    if (!device_is_ready(red_led.port) || !device_is_ready(green_led.port))
    {
        printk("One or more LED devices not ready!\n");
        return -1;
    }

    gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_INACTIVE);

    printk("GPIOs initialized successfully\n");
    return 0;
}

// Ring buffer -lisäysfunktio
void ring_buffer_put(struct ring_buffer *rb, const char *msg)
{
    for (int i = 0; msg[i] != '\0'; i++)
    {
        rb->buffer[rb->head] = msg[i];
        rb->head = (rb->head + 1) % MAX_MSG_LEN;
        rb->count++;
        // Vältetään ylivuoto
        if (rb->count > MAX_MSG_LEN)
        {
            rb->count = MAX_MSG_LEN;
            rb->tail = (rb->tail + 1) % MAX_MSG_LEN;
        }
    }
}

// Ring buffer -luku funktio
int ring_buffer_get(struct ring_buffer *rb, char *msg)
{
    if (rb->count == 0)
    {
        return -1; // Puskuri on tyhjä
    }

    int i = 0;
    while (rb->count > 0 && i < MAX_MSG_LEN - 1)
    {
        msg[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % MAX_MSG_LEN;
        rb->count--;
        i++;
    }
    msg[i] = '\0'; // Lopetetaan merkkijono
    return 0;
}

// UART-vastaanottotehtävä
void uart_receive_task(void *p1, void *p2, void *p3)
{
    char rc;
    printk("UART Receive Task Started\n");

    while (true)
    {
        // Tarkistetaan UART-tulo
        if (uart_poll_in(uart_dev, &rc) == 0)
        {
            // Tarkistetaan, että merkki ei ole pieni kirjain eikä piste
            assert(!(rc >= 'a' && rc <= 'z'));                                                                                         // Väärät merkit: pienet kirjaimet
            assert(rc != '.' && rc != '!' && rc != '@' && rc != '#' && rc != '$' && rc != '%' && rc != '^' && rc != '&' && rc != '*'); // Väärät merkit: piste ja muut erikoismerkit

            // Lähetetään merkki takaisin terminaaliin
            uart_poll_out(uart_dev, rc);

            // Tallennetaan merkki viestipuskuriin
            uart_msg[uart_msg_index++] = rc;

            // Tarkistetaan viestin loppu (rivinvaihto '\n' tai '\r')
            if (rc == '\n' || rc == '\r' || uart_msg_index >= MAX_MSG_LEN)
            {
                uart_msg[uart_msg_index - 1] = '\0'; // Lopetetaan viesti

                // Käsitellään 'D' komento debug-tilan vaihtamiseksi
                if (strcmp(uart_msg, "D,1") == 0)
                {
                    debug_enabled = true;
                    printk("Debugging enabled\n");
                }
                else if (strcmp(uart_msg, "D,0") == 0)
                {
                    debug_enabled = false;
                    printk("Debugging disabled\n");
                }
                else
                {
                    // Tarkistetaan, onko viesti aikamerkkijono (12-numeroa)
                    if (strlen(uart_msg) == 6 && strspn(uart_msg, "0123456789") == 6)
                    {
                        int parsed_time = time_parse(uart_msg);
                        if (parsed_time >= 0)
                        {
                            printk("Valid time string received: %s\n", uart_msg);
                            set_traffic_light_timer(parsed_time);
                        }
                        else
                        {
                            printk("Invalid time string received: %s\n", uart_msg);
                        }
                    }
                    else
                    {
                        // Ei kelvollinen aikamerkkijono; käsitellään komento-viestinä
                        printk("Received command: %s\n", uart_msg);
                        k_msgq_put(&uart_msgq, uart_msg, K_NO_WAIT);
                    }

                    uart_sequence_end_time = k_cycle_get_32(); // Loppuaika, kun viesti on vastaanotettu
                    uint32_t duration_cycles = uart_sequence_end_time - uart_sequence_start_time;
                    uint32_t duration_ns = k_cyc_to_ns_floor64(duration_cycles);
                    uint32_t duration_us = duration_ns / 1000;
                    DEBUG_PRINT("UART sequence received in %u us\n", duration_us); // Raportoidaan vastaanottoaika
                }

                // Nollataan viesti-indeksi seuraavaa viestiä varten
                uart_msg_index = 0;
            }
        }
        k_msleep(10); // Pieni viive, jotta vältetään kiireinen odotus
    }
}

// Traffic light timer handler
void traffic_light_timer_handler(struct k_timer *dummy)
{
    printk("Traffic light timer expired. Triggering light change.\n");
    // Here, you could trigger some behavior, like turning on a specific light.
    // For example, switch on the red light for a while.
    gpio_pin_set_dt(&red_led, 1);
    k_msleep(1000);
    gpio_pin_set_dt(&red_led, 0);
}

// Set traffic light timer function
void set_traffic_light_timer(int seconds)
{
    // Initialize the timer if it has not been already.
    k_timer_init(&traffic_light_timer, traffic_light_timer_handler, NULL);

    // Start the timer.
    printk("Setting traffic light timer for %d seconds.\n", seconds);
    k_timer_start(&traffic_light_timer, K_SECONDS(seconds), K_NO_WAIT);
}

// Dispatcher-tehtävä
void dispatcher_task(void *p1, void *p2, void *p3)
{
    printk("Dispatcher Task Started\n");

    char msg[MAX_MSG_LEN];
    int command_count;
    command_t parsed_commands[MAX_COMMANDS]; // Käytä TrafficParserin command_t-rakennetta
    int repeat_times = 1;

    while (true)
    {
        // Hae seuraava viesti viestijonosta
        if (k_msgq_get(&uart_msgq, msg, K_NO_WAIT) == 0)
        {
            printk("Dispatcher received message: %s\n", msg);

            uint32_t start_time = k_cycle_get_32();

            // Nollaa total_duration_us
            k_mutex_lock(&total_duration_mutex, K_FOREVER);
            total_duration_us = 0;
            k_mutex_unlock(&total_duration_mutex);

            // Tarkista, onko viestissä 'T' toistoja varten
            char *t_ptr = strchr(msg, 'T');
            if (t_ptr != NULL)
            {
                // Kopioi sekvenssi ennen 'T'-merkkiä
                int sequence_len = t_ptr - msg;
                char sequence[MAX_MSG_LEN];
                strncpy(sequence, msg, sequence_len);
                sequence[sequence_len] = '\0';

                // Lue toistojen määrä 'T'-merkin jälkeen
                if (sscanf(t_ptr, "T,%d", &repeat_times) != 1)
                {
                    DEBUG_PRINT("Invalid repeat format in message\n");
                    repeat_times = 1; // Oletusarvo 1, jos jäsentäminen epäonnistuu
                }

                // Varmista, että repeat_times on kohtuullisissa rajoissa
                if (repeat_times < 1 || repeat_times > 100)
                {
                    printk("Repeat times out of bounds: %d\n", repeat_times);
                    repeat_times = 1;
                }

                // Käytä sequence_parse-funktiota komentosarjan jäsentämiseen
                int parse_result = sequence_parse(sequence, parsed_commands, &command_count);
                if (parse_result != 0)
                {
                    printk("Error parsing sequence: %d\n", parse_result);
                    continue; // Ohita tämä viesti
                }
            }
            else
            {
                // Ei 'T'-merkkiä, käytä koko viestiä sekvenssinä
                repeat_times = 1;
                int parse_result = sequence_parse(msg, parsed_commands, &command_count);
                if (parse_result != 0)
                {
                    printk("Error parsing sequence: %d\n", parse_result);
                    continue; // Ohita tämä viesti
                }
            }

            // Toista sekvenssi määritetyn määrän kertoja
            for (int r = 0; r < repeat_times; r++)
            {
                // Käsittele jokainen jäsennetty komento
                for (int i = 0; i < command_count; i++)
                {
                    char color = parsed_commands[i].color;
                    int duration = parsed_commands[i].duration;

                    if (duration <= 0)
                    {
                        duration = DEFAULT_DURATION_MS; // Käytä oletuskestoa, jos ei määritelty
                    }

                    struct led_item *led_data = k_malloc(sizeof(struct led_item));
                    if (led_data == NULL)
                    {
                        printk("Error: Memory allocation failed for led_data\n");
                        continue; // Ohita tämä komento
                    }

                    led_data->duration = duration;

                    // Lisää led_data oikeaan FIFOon
                    k_mutex_lock(&led_mutex, K_FOREVER);
                    switch (color)
                    {
                    case 'R':
                        k_fifo_put(&red_fifo, led_data);
                        k_condvar_signal(&red_condvar);
                        break;
                    case 'G':
                        k_fifo_put(&green_fifo, led_data);
                        k_condvar_signal(&green_condvar);
                        break;
                    case 'Y':
                        k_fifo_put(&yellow_fifo, led_data);
                        k_condvar_signal(&yellow_condvar);
                        break;
                    default:
                        DEBUG_PRINT("Unknown color received: %c\n", color);
                        k_free(led_data);
                        break;
                    }
                    k_mutex_unlock(&led_mutex);
                    printk("Parsed command - Color: %c, Duration: %d ms\n", color, duration);

                    // Lisää komento command_queue-jonoon
                    struct command_item *cmd_item = k_malloc(sizeof(struct command_item));
                    if (cmd_item == NULL)
                    {
                        printk("Error: Memory allocation failed for cmd_item\n");
                        continue; // Ohita tämä komento
                    }

                    cmd_item->color = color;
                    k_fifo_put(&command_queue, cmd_item);
                }

                // Alusta sekvenssin semafori
                k_sem_reset(&sequence_sem);

                // Odota, että kaikki valotehtävät suoritetaan
                for (int i = 0; i < command_count; i++)
                {
                    k_sem_take(&sequence_sem, K_FOREVER);
                }
            }

            // Laske ja tulosta koko sekvenssin kesto
            k_mutex_lock(&total_duration_mutex, K_FOREVER);
            DEBUG_PRINT("Total sequence duration: %u us\n", total_duration_us);
            k_mutex_unlock(&total_duration_mutex);

            uint32_t end_time = k_cycle_get_32();
            uint32_t duration_cycles = end_time - start_time;
            uint32_t duration_ns = k_cyc_to_ns_floor64(duration_cycles);
            dispatcher_processing_time_us = duration_ns / 1000;

            DEBUG_PRINT("Dispatcher processed sequence in %u us\n", dispatcher_processing_time_us);
        }

        k_msleep(10); // Pieni viive kiireisen odotuksen välttämiseksi
    }
}

// Punaisen valon tehtävä
void red_light_task(void *p1, void *p2, void *p3)
{
    printk("Red Light Task Started\n");
    while (true)
    {
        // Odota, että FIFOssa on dataa
        k_mutex_lock(&led_mutex, K_FOREVER);
        while (k_fifo_is_empty(&red_fifo))
        {
            k_condvar_wait(&red_condvar, &led_mutex, K_FOREVER);
        }
        k_mutex_unlock(&led_mutex);

        // Odota vuoroasi command_queue-jonossa
        struct command_item *cmd_item = NULL;
        while (true)
        {
            cmd_item = k_fifo_peek_head(&command_queue);
            if (cmd_item != NULL && cmd_item->color == 'R')
            {
                // On meidän vuoro
                k_fifo_get(&command_queue, K_NO_WAIT);
                k_free(cmd_item);
                break;
            }
            else
            {
                // Ei meidän vuoro, odotetaan hetki
                k_msleep(1);
            }
        }

        // Prosessoi oma FIFO
        struct led_item *item;
        k_mutex_lock(&led_mutex, K_FOREVER);
        item = k_fifo_get(&red_fifo, K_NO_WAIT);
        k_mutex_unlock(&led_mutex);

        if (item != NULL)
        {
            uint32_t start_time = k_cycle_get_32();

            k_mutex_lock(&light_mutex, K_FOREVER);
            printk("Red light ON for %d ms\n", item->duration);
            gpio_pin_set_dt(&green_led, 0); // Varmista, että vihreä LED on pois päältä
            gpio_pin_set_dt(&red_led, 1);   // Sytytä punainen LED
            k_msleep(item->duration);
            gpio_pin_set_dt(&red_led, 0); // Sammuta punainen LED
            printk("Red light OFF\n");
            k_mutex_unlock(&light_mutex);

            uint32_t end_time = k_cycle_get_32();
            uint32_t duration_cycles = end_time - start_time;
            uint32_t duration_ns = k_cyc_to_ns_floor64(duration_cycles);
            uint32_t duration_us = duration_ns / 1000;

            // Päivitä total_duration_us
            k_mutex_lock(&total_duration_mutex, K_FOREVER);
            total_duration_us += duration_us;
            k_mutex_unlock(&total_duration_mutex);

            // Tulosta yksittäisen tehtävän aika
            printk("Red task duration: %u us\n", duration_us);

            // Ilmoita dispatcherille, että tehtävä on suoritettu
            k_sem_give(&sequence_sem);

            k_free(item);
        }
    }
}

// Vihreän valon tehtävä
void green_light_task(void *p1, void *p2, void *p3)
{
    printk("Green Light Task Started\n");
    while (true)
    {
        // Odota, että FIFOssa on dataa
        k_mutex_lock(&led_mutex, K_FOREVER);
        while (k_fifo_is_empty(&green_fifo))
        {
            k_condvar_wait(&green_condvar, &led_mutex, K_FOREVER);
        }
        k_mutex_unlock(&led_mutex);

        // Odota vuoroasi command_queue-jonossa
        struct command_item *cmd_item = NULL;
        while (true)
        {
            cmd_item = k_fifo_peek_head(&command_queue);
            if (cmd_item != NULL && cmd_item->color == 'G')
            {
                // On meidän vuoro
                k_fifo_get(&command_queue, K_NO_WAIT);
                k_free(cmd_item);
                break;
            }
            else
            {
                // Ei meidän vuoro, odotetaan hetki
                k_msleep(1);
            }
        }

        // Prosessoi oma FIFO
        struct led_item *item;
        k_mutex_lock(&led_mutex, K_FOREVER);
        item = k_fifo_get(&green_fifo, K_NO_WAIT);
        k_mutex_unlock(&led_mutex);

        if (item != NULL)
        {
            uint32_t start_time = k_cycle_get_32();

            k_mutex_lock(&light_mutex, K_FOREVER);
            printk("Green light ON for %d ms\n", item->duration);
            gpio_pin_set_dt(&red_led, 0);   // Varmista, että punainen LED on pois päältä
            gpio_pin_set_dt(&green_led, 1); // Sytytä vihreä LED
            k_msleep(item->duration);
            gpio_pin_set_dt(&green_led, 0); // Sammuta vihreä LED
            printk("Green light OFF\n");
            k_mutex_unlock(&light_mutex);

            uint32_t end_time = k_cycle_get_32();
            uint32_t duration_cycles = end_time - start_time;
            uint32_t duration_ns = k_cyc_to_ns_floor64(duration_cycles);
            uint32_t duration_us = duration_ns / 1000;

            // Päivitä total_duration_us
            k_mutex_lock(&total_duration_mutex, K_FOREVER);
            total_duration_us += duration_us;
            k_mutex_unlock(&total_duration_mutex);

            // Tulosta yksittäisen tehtävän aika
            printk("Green task duration: %u us\n", duration_us);

            // Ilmoita dispatcherille, että tehtävä on suoritettu
            k_sem_give(&sequence_sem);

            k_free(item);
        }
    }
}

// Keltaisen valon tehtävä
void yellow_light_task(void *p1, void *p2, void *p3)
{
    printk("Yellow Light Task Started\n");
    while (true)
    {
        // Odota, että FIFOssa on dataa
        k_mutex_lock(&led_mutex, K_FOREVER);
        while (k_fifo_is_empty(&yellow_fifo))
        {
            k_condvar_wait(&yellow_condvar, &led_mutex, K_FOREVER);
        }
        k_mutex_unlock(&led_mutex);

        // Odota vuoroasi command_queue-jonossa
        struct command_item *cmd_item = NULL;
        while (true)
        {
            cmd_item = k_fifo_peek_head(&command_queue);
            if (cmd_item != NULL && cmd_item->color == 'Y')
            {
                // On meidän vuoro
                k_fifo_get(&command_queue, K_NO_WAIT);
                k_free(cmd_item);
                break;
            }
            else
            {
                // Ei meidän vuoro, odotetaan hetki
                k_msleep(1);
            }
        }

        // Prosessoi oma FIFO
        struct led_item *item;
        k_mutex_lock(&led_mutex, K_FOREVER);
        item = k_fifo_get(&yellow_fifo, K_NO_WAIT);
        k_mutex_unlock(&led_mutex);

        if (item != NULL)
        {
            uint32_t start_time = k_cycle_get_32();

            k_mutex_lock(&light_mutex, K_FOREVER);
            printk("Yellow light (Red + Green) ON for %d ms\n", item->duration);
            gpio_pin_set_dt(&red_led, 1);   // Sytytä punainen LED
            gpio_pin_set_dt(&green_led, 1); // Sytytä vihreä LED
            k_msleep(item->duration);
            gpio_pin_set_dt(&red_led, 0);   // Sammuta punainen LED
            gpio_pin_set_dt(&green_led, 0); // Sammuta vihreä LED
            printk("Yellow light OFF\n");
            k_mutex_unlock(&light_mutex);

            uint32_t end_time = k_cycle_get_32();
            uint32_t duration_cycles = end_time - start_time;
            uint32_t duration_ns = k_cyc_to_ns_floor64(duration_cycles);
            uint32_t duration_us = duration_ns / 1000;

            // Päivitä total_duration_us
            k_mutex_lock(&total_duration_mutex, K_FOREVER);
            total_duration_us += duration_us;
            k_mutex_unlock(&total_duration_mutex);

            // Tulosta yksittäisen tehtävän aika
            printk("Yellow task duration: %u us\n", duration_us);

            // Ilmoita dispatcherille, että tehtävä on suoritettu
            k_sem_give(&sequence_sem);

            k_free(item);
        }
    }
}

int main(void)
{
    int ret = init_uart();
    if (ret != 0)
    {
        printk("UART initialization failed!\n");
        return ret;
    }

    ret = init_gpio();
    if (ret != 0)
    {
        printk("GPIO initialization failed!\n");
        return ret;
    }

    // Odota, että kaikki alustuu
    k_msleep(100);

    printk("Started serial read example\n");

    // Alusta condition variablet ja muteksit
    k_condvar_init(&red_condvar);
    k_condvar_init(&yellow_condvar);
    k_condvar_init(&green_condvar);
    k_mutex_init(&led_mutex);
    k_mutex_init(&light_mutex); // Uusi muteksi valojen hallintaan
    k_mutex_init(&total_duration_mutex);
    k_sem_init(&sequence_sem, 0, UINT_MAX);

    // Luo säikeet tehtäville
    k_thread_create(&uart_receive_thread_data, uart_receive_stack, STACK_SIZE,
                    uart_receive_task, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&dispatcher_thread_data, dispatcher_stack, STACK_SIZE,
                    dispatcher_task, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&red_task_thread_data, red_task_stack, STACK_SIZE,
                    red_light_task, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&green_task_thread_data, green_task_stack, STACK_SIZE,
                    green_light_task, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&yellow_task_thread_data, yellow_task_stack, STACK_SIZE,
                    yellow_light_task, NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    return 0;
}
