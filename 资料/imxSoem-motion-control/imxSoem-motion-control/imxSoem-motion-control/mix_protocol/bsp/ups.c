#include "ups.h"
int UPS_power_check(void)
{
    return GPIO_ReadPinInput(GPIO5, 24);
}

void UPS_GPIO_Init(void)
{
    gpio_pin_config_t gpio_input_config = {kGPIO_DigitalInput, 0, kGPIO_NoIntmode};
    gpio_pin_config_t gpio_output_config = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};

    GPIO_PinInit(UPS_LIMIT_GPIO_PORT, UPS_LIMIT_GPIO_PIN, &gpio_input_config);
    GPIO_PinInit(UPS_OUTPUT_GPIO_PORT, UPS_OUTPUT_GPIO_PIN, &gpio_output_config);
    GPIO_WritePinOutput(UPS_OUTPUT_GPIO_PORT, UPS_OUTPUT_GPIO_PIN, 0);
}