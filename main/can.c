#include <string.h>

#include "driver/gpio.h"
#include "driver/can.h"
#include "can.h"

void CAN_Open()
{
    //Initialize configuration structures using macro initializers
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, CAN_MODE_NORMAL);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
    can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

    //Install CAN driver
    if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        printf("Driver installed\n");
    }
    else
    {
        printf("Failed to install driver\n");
        return;
    }

    //Start CAN driver
    if (can_start() == ESP_OK)
    {
        printf("Driver started\n");
    }
    else
    {
        printf("Failed to start driver\n");
        return;
    }
}

void CAN_Close()
{
    //Stop the CAN driver
    if (can_stop() == ESP_OK)
    {
        printf("Driver stopped\n");
    }
    else
    {
        printf("Failed to stop driver\n");
        return;
    }

    //Uninstall the CAN driver
    if (can_driver_uninstall() == ESP_OK)
    {
        printf("Driver uninstalled\n");
    }
    else
    {
        printf("Failed to uninstall driver\n");
        return;
    }
}

// ESP32 does not care about extended frame messages.
uint8_t CAN_ReceiveMsg(CanMessage_t * msg){
    can_message_t _msg;

    esp_err_t err = can_receive(&_msg, portMAX_DELAY);

    if(err == ESP_OK){
        msg->id = _msg.identifier;
        msg->len = _msg.data_length_code;
        memmove(msg->payload, _msg.data, 8);

        return 0; //sucessful message RX
    }
    return 1; //something terrible happened
}

// TODO: understand error codes for this one - 255 appears to be sucess and 0 failure.
uint8_t CAN_SendMsg(CanMessage_t * msg){
    can_message_t _msg;
    _msg.identifier = msg->id;
    _msg.data_length_code = msg->len;
    memmove(_msg.data, msg->payload, 8);

    esp_err_t err = can_transmit(&_msg, portMAX_DELAY);
    if(err == ESP_OK){
        err = can_clear_transmit_queue();
        if(err == ESP_OK){
            return 0;
        }
    }
    return 1;
}

// Return the number of pending received frames
uint8_t CAN_MessagePending(){
    can_status_info_t status;
    esp_err_t err = can_get_status_info(&status);
    if(err){
        printf("Failed to get CAN status");
        return 1;
    }
    return status.msgs_to_tx;
}

//sets inbox filter to only accept "id"
void CAN_SetFilter0(uint32_t id){
    can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
    f_config.single_filter = true;
    f_config.acceptance_mask = id; //TODO: theres some magic here. Make sure the mask is a simple AND.

    CAN_Close();

    //Initialize configuration structures using macro initializers
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, CAN_MODE_NORMAL);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();

    //Install CAN driver
    printf("Setting filter by reinstalling and starting the driver\n");
    if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        printf("Driver reinstalled\n");
    }
    else
    {
        printf("Failed to reinstall driver\n");
        return;
    }

    //Start CAN driver
    if (can_start() == ESP_OK)
    {
        printf("Driver restarted\n");
    }
    else
    {
        printf("Failed to restart driver\n");
        return;
    }

    printf("Driver filter sucessfully set\n");
}

void CAN_ResetFilter0(){
    printf("Resetting Filter\n");
    CAN_Close();
    CAN_Open();
    printf("Filter Reset\n");
}

void CAN_FlushReceiveFifo(){
    esp_err_t err = can_clear_receive_queue();
    if(err == ESP_OK){
        printf("Sucessfully cleared the RX queue\n");
        return;
    }
    if(err == ESP_ERR_INVALID_STATE){
        printf("CAN driver is not installed\n");
    }
    printf("Failed to clear RX queue\n");
};

//not implementing
void CAN_Debug(uint8_t on, FILE *debug_file){
    return;
}
void CAN_UseStdId(){
    return;
}
void CAN_UseExtId(){
    return;
}