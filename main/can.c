#include <string.h>

#include "driver/gpio.h"
#include "driver/can.h"
#include "can.h"

void CAN_Open()
{
    //Initialize configuration structures using macro initializers
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_26, GPIO_NUM_32, CAN_MODE_NORMAL);
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
        // printf("%s:%d driver stopped\n", __FILE__, __LINE__);
    }
    else
    {
        // printf("%s:%d FAILED: driver stopped\n", __FILE__, __LINE__);
        return;
    }

    //Uninstall the CAN driver
    if (can_driver_uninstall() == ESP_OK)
    {
        // printf("%s:%d driver uninstalled\n", __FILE__, __LINE__);
    }
    else
    {
        // printf("%s:%d FAILED: driver uninstalled\n", __FILE__, __LINE__);
        return;
    }
}

// For recieving a message. Blocks for up to {timeout} ms. 0 for failure, 1 for success.
uint8_t CAN_ReceiveMsg(CanMessage_t * msg, int timeout){
    can_message_t _msg;
    printf("%s:%d dequeueing message\n", __FILE__, __LINE__);
    esp_err_t err = can_receive(&_msg, portTICK_PERIOD_MS * timeout);
    printf("%s:%d dequeued message sucessfully\n", __FILE__, __LINE__);
    if(err == ESP_OK){
        msg->id = _msg.identifier;
        msg->len = _msg.data_length_code;
        memmove(msg->payload, _msg.data, 8);
        printf("%s:%d successfully got the message\n", __FILE__, __LINE__);
        return 1; //sucessful message RX
    }
    printf("%s:%d FAILED: fetching message\n", __FILE__, __LINE__);
    return 0; //something terrible happened
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
            printf("%s:%d message sent\n", __FILE__, __LINE__);
            return 1;
        }
    }
    printf("%s:%d FAILED: message sent\n", __FILE__, __LINE__);
    return 0;
}

//sets inbox filter to only accept "id"
void CAN_SetFilter0(uint32_t id){
    // printf("%s:%d setting filter to %3x\n", __FILE__, __LINE__, id);
    can_filter_config_t f_config = {.acceptance_code = (id << 21),
                                    .acceptance_mask = ~(CAN_STD_ID_MASK << 21),
                                    .single_filter = true};
    CAN_Close();

    //Initialize configuration structures using macro initializers
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_26, GPIO_NUM_32, CAN_MODE_NORMAL);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();

    // printf("%s:%d installing filter\n", __FILE__, __LINE__);
    //Install CAN driver
    if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        // printf("%s:%d filter installed\n", __FILE__, __LINE__);
    }
    else
    {
        // printf("%s:%d FAILED: installing filter\n", __FILE__, __LINE__);
        return;
    }

    //Start CAN driver
    if (can_start() == ESP_OK)
    {
        // printf("%s:%d driver started\n", __FILE__, __LINE__);
    }
    else
    {
        // printf("%s:%d FAILED: driver started\n", __FILE__, __LINE__);
        return;
    }

    // printf("%s:%d sucessfully installed filter\n", __FILE__, __LINE__);
}

void CAN_ResetFilter0(){
    // printf("%s:%d resetting filter\n", __FILE__, __LINE__);
    CAN_SetFilter0(0x201);
    CAN_FlushReceiveFifo();
    //printf("%s:%d filter reset\n", __FILE__, __LINE__);
}

void CAN_FlushReceiveFifo(){
    esp_err_t err = can_clear_receive_queue();
    if(err == ESP_OK){
        // printf("%s:%d cleared RX queue\n", __FILE__, __LINE__);
        return;
    }
    if(err == ESP_ERR_INVALID_STATE){
        // printf("%s:%d CAN driver is not installed\n", __FILE__, __LINE__);
    }
    // printf("%s:%d Failed to clear RX queue\n", __FILE__, __LINE__);
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