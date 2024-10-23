
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/mgmt/hawkbit.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/reboot.h>

// #define SSID "Dans iPhone"
// #define PSK  "_danromm"
#define SSID "Rom_Household"
#define PSK  "Rom12345"

#define SLEEP_TIME 10000U

struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

//Global variables for network management
bool connection_established=false;
static struct net_mgmt_event_callback wifi_cb;

//Enable Logging
LOG_MODULE_REGISTER(main);

//Get WDT hardware instance from Devicetree API
const struct device *const wdt_dev = DEVICE_DT_GET(DT_ALIAS(watchdog0));

//Create timeout configuration data structure
struct wdt_timeout_cfg wdt_cfg = {
	// Reset SoC when watchdog timer expires. 
	.flags = WDT_FLAG_RESET_SOC,
	// Set Watchdog window to 1 minute
	.window.min = 0U,
	.window.max = 60000U,
};

static void request_connection(struct net_iface* iface){
	//Data needed to request a Wi-Fi connection
	struct wifi_connect_req_params wifi_params;
	/* Defaults */
	wifi_params.band = WIFI_FREQ_BAND_UNKNOWN;
	wifi_params.channel = WIFI_CHANNEL_ANY;
	wifi_params.mfp = WIFI_MFP_OPTIONAL;
	/*Network Specific*/
	wifi_params.ssid=SSID;
	wifi_params.ssid_length=strlen(SSID);
	wifi_params.psk=PSK;
	wifi_params.psk_length=strlen(PSK);
	wifi_params.security=WIFI_SECURITY_TYPE_PSK;

	int ret=-1;
	//Request Wi-Fi conection
	ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params, sizeof(struct wifi_connect_req_params));
	if(ret != 0){
		printk("Connection request failed with error: %d\n", ret);
	}
    else {
		printk("Connection request successful\n");
	}
}

//Custom function used in network management event callback
static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint32_t mgmt_event,
                               struct net_if *iface)
{
	printk("Event Handler Called\n");
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        struct wifi_status *status = (struct wifi_status *)cb->info;
        if (status->status == 0) {
            printk("Wi-Fi connected successfully\n");
			connection_established=true;
            // Wi-Fi connected, proceed with your program logic.
        } else {
			//Wi-Fi connection failed, try again
            printk("Wi-Fi connection failed\n");
			connection_established=false;
			request_connection(iface);
        }
    } else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
		//Wi-Fi disconnected, try again
        LOG_INF("Wi-Fi disconnected\n");
		connection_established=false;
		request_connection(iface);
    }
}

//Application function
int application(int wdt_channel_id, struct net_if* iface){
	if(connection_established){
		//Feed Watchdog to ensure device is not reset
		int ret = wdt_feed(wdt_dev, wdt_channel_id);
		//If ret isn't 0, something failed and firmware must be rolled back
		if(ret != 0){
			return ret;
		}
		//Toggle LED
		ret = gpio_pin_toggle_dt(&led0);
		//If ret isn't 0, something failed and firmware must be rolled back
		if(ret != 0){
			return ret;
		}
	}
	else{
		request_connection(iface);
	}
	k_msleep(SLEEP_TIME);
	return -1;
}

int main(void){

	//Get Wi-Fi interface
    struct net_if* iface = net_if_get_first_wifi();

	//Create and register network management event callback
	net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler,
                                 NET_EVENT_WIFI_CONNECT_RESULT |
                                 NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);

	//Configure Watchdog instance
	int wdt_channel_id = wdt_install_timeout(wdt_dev, &wdt_cfg);
    if (wdt_channel_id < 0) {
        printk("Failed to install watchdog timeout.\n");
    }

	//Initiate Wi-Fi connect request
	request_connection(iface);

	//Halt program until a Wi-Fi connection is established
	while(!connection_established){
		k_msleep(1000);
	}

	//Validate GPIO port is ready for use
	if(!gpio_is_ready_dt(&led0)){
		return 0;
	}

	//Configure GPIO port as an output
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);

	//Perform 'self-check' - run through Application code once
	int ret = application(wdt_channel_id, iface);

	//If 'self-check' fails, reboot system without confirming image
	if(ret != 0){
		sys_reboot(SYS_REBOOT_COLD);
	}

	//Initialize target device to be hawkBit-ready
	ret = hawkbit_init();

	//Begin running hawkBit's autohandler
	hawkbit_autohandler();

	// Start the watchdog
	wdt_setup(wdt_dev, WDT_OPT_PAUSE_IN_SLEEP);
	//Continuously toggle LED, every 'SLEEP_TIME' ms
	while(1){
		application(wdt_channel_id, iface);
	}
	return 0;
}