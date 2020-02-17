#include "esphome.h"
#include <Chrono.h>

using namespace std;

class EnbraWmbus08 : public Component, public uart::UARTDevice, public Sensor
{
public:
    // constructor
    EnbraWmbus08(uart::UARTComponent *parent, std::string wm_address) : uart::UARTDevice(parent) {
        strcpy(serialNumberInitial, wm_address.c_str());
    }

    char serialNumberInitial[10];
    char serialNumber[10];
    uint8_t temp_byte = 0;
    uint8_t *temp_byte_pointer = &temp_byte;
    uint8_t uart_buffer_[512]{0};
    uint16_t uart_counter = 0;
    char uart_message[550];
    char temp_string[10];    

    Chrono myChrono;

    void setup() override
    {
        // This will be called by App.setup()
    }

    void loop() override
    {
        bool have_message = read_message();
    }

    bool read_message()
    {
        if (myChrono.hasPassed(70)) // run if 70ms have passed.
        {                       
            myChrono.restart(); // restart the Chrono

            uart_message[0] = '\0';
            strcpy(uart_message, "");

            if (uart_counter > 0)
            {
                for (uint16_t i = 0; i < uart_counter; i++)
                {
                    sprintf(temp_string, "%02X ", uart_buffer_[i]);
                    strcat(uart_message, temp_string);
                }

                ESP_LOGD("uart", "Data: %s", uart_message);
                parseMessage();
            }

            uart_counter = 0;
        }

        while (available() >= 1)
        {
            read_byte(this->temp_byte_pointer);

            uart_buffer_[uart_counter] = temp_byte;
            uart_counter++;

            if (uart_counter == 512)
            {
                uart_counter = 0;
            }

            myChrono.restart();
        }

        return false;
    }

    void parseMessage()
    {
        uint8_t telegramLength = uart_buffer_[0];
        uint8_t operation = uart_buffer_[1];
        uint8_t manufacturer01 = uart_buffer_[2];
        uint8_t manufacturer02 = uart_buffer_[3];        
        char consuption[8];

        ESP_LOGD("telegram", "Telegram length: %d", telegramLength);

        if ((telegramLength + 1) == uart_counter &&
            operation == 0x44 &&
            manufacturer01 == 0x14 &&
            manufacturer02 == 0x86)
        {
            ESP_LOGD("telegram", "%s", "ENBRA message");

            serialNumber[0] = '\0';
            strcpy(serialNumber, "");
            temp_string[0] = '\0';
            strcpy(temp_string, "");

            // read Identification
            for (uint16_t i = 7; i > 3; i--)
            {
                sprintf(temp_string, "%02X", uart_buffer_[i]);
                strcat(serialNumber, temp_string);
            }

            ESP_LOGD("telegram", "Identification: %s", serialNumber);

            // read consumption
            consuption[0] = '\0';
            strcpy(consuption, "");
            temp_string[0] = '\0';
            strcpy(temp_string, "");
            for (uint16_t i = 14; i > 10; i--)
            {
                sprintf(temp_string, "%02X", uart_buffer_[i]);
                strcat(consuption, temp_string);
            }

            uint32_t consuptionNumber;
            sscanf(consuption, "%x", &consuptionNumber);
            consuptionNumber = consuptionNumber / 3;
            
            if(strcmp(serialNumberInitial, serialNumber) == 0)
            {
                publish_state(consuptionNumber);
            }
            
            ESP_LOGD("telegram", "Consumption: %d", consuptionNumber);
        }
    }
};
