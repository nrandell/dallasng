#pragma once

#include "esphome/components/sensor/sensor.h"
#include "dallasng_component.h"
#include "utils/Placeholder.h"

namespace esphome
{
    namespace dallasng
    {
        static const char *const TAG = "dallasng.sensor";
        class DallasNgTemperatureSensor : public sensor::Sensor
        {
        public:
            void set_parent(DallasNgComponent *parent)
            {
                parent_ = parent;
            }

            void set_address(uint64_t address)
            {
                address_ = address;
                memcpy(&id_[0], &address, sizeof(id_));
            }

            uint64_t get_address() const
            {
                return address_;
            }

            const std::string get_address_name()
            {
                if (address_name_.empty())
                {
                    address_name_ = std::string("0x") + format_hex(this->address_);
                }

                return address_name_;
            }

            void set_index(uint8_t index)
            {
                index_ = index;
            }

            optional<uint8_t> get_index() const
            {
                return index_;
            }

            uint8_t get_resolution() const
            {
                return resolution_;
            }

            void set_resolution(uint8_t resolution)
            {
                resolution_ = resolution;
            }

            void dumpScratchpad(DSTherm::Scratchpad &scratchpad)
            {
                auto raw = scratchpad.getRaw();
                ESP_LOGV(TAG, "Scratchpad: %02x%02x%02x%02x%02x%02x%02x%02x%02x",
                         raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7], raw[8]);
            }

            bool setup()
            {
                auto sensor = DSTherm(*parent_->one_wire_);
                Placeholder<DSTherm::Scratchpad> scratchpad;

                auto result = sensor.readScratchpad(id_, &scratchpad);
                if (result != OneWireNg::EC_SUCCESS)
                {
                    ESP_LOGW(TAG, "'%s' failed to read scratchpad: %d", get_name().c_str(), result);
                    return false;
                }
                dumpScratchpad(scratchpad);

                DSTherm::Scratchpad &s = scratchpad;
                DSTherm::Resolution resolution = (DSTherm::Resolution)(resolution_ - 9);
                ESP_LOGI(TAG, "Resolution %d (%d)", resolution_, resolution);
                s.setResolution(resolution);

                result = s.writeScratchpad();
                if (result != OneWireNg::EC_SUCCESS)
                {
                    ESP_LOGW(TAG, "'%s' failed to write scratchpad: %d", get_name().c_str(), result);
                    return false;
                }
                dumpScratchpad(scratchpad);

                result = sensor.copyScratchpad(id_);
                if (result != OneWireNg::EC_SUCCESS)
                {
                    ESP_LOGW(TAG, "'%s' failed to write scratchpad: %d", get_name().c_str(), result);
                    return false;
                }

                return true;
            }

            bool try_get_temperature_c(float *value)
            {
                auto sensor = DSTherm(*parent_->one_wire_);
                Placeholder<DSTherm::Scratchpad> scratchpad;
                auto result = sensor.readScratchpad(id_, &scratchpad);
                if (result != OneWireNg::EC_SUCCESS)
                {
                    ESP_LOGW(TAG, "'%s' failed to read scratchpad: %d", get_name().c_str(), result);
                    return false;
                }
                dumpScratchpad(scratchpad);

                DSTherm::Scratchpad &s = scratchpad;
                long temp = s.getTemp();
                *value = (float)temp / 1000.0f;
                ESP_LOGD(TAG, "'%s' got temperature as %ld (%f)", get_name().c_str(), temp, *value);
                return true;
            }

            uint16_t millis_to_wait_for_conversion() const
            {
                return DSTherm::getConversionTime((DSTherm::Resolution)(resolution_ - 9));
            }

        private:
            DallasNgComponent *parent_;
            OneWireNg::Id id_;
            uint64_t address_;
            uint8_t resolution_ = 12;
            std::string address_name_;
            optional<uint8_t> index_;
        };
    }
}