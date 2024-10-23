#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME 2000U

struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void){

	//Validate GPIO port is ready for use
	if(!gpio_is_ready_dt(&led0)){
		return 0;
	}

	//Configure GPIO port as an output
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);

	//Continuously toggle LED, every ’SLEEP_TIME’ ms
	while(1){
		gpio_pin_toggle_dt(&led0);
		k_msleep(SLEEP_TIME);
	}
	return 0;
}