#include "dallasng_component.h"
#include "dallasng_temperature_sensor.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace dallasng
  {

    void DallasNgComponent::setup()
    {
      ESP_LOGCONFIG(TAG, "Setting up DallasComponent...");
      one_wire_->searchReset();
      ESP_LOGI(TAG, "One wire using pin %d", pin_->get_pin());
      OneWireNg::Id id;
      one_wire_->searchReset();
      while (one_wire_->search(id, false) == OneWireNg::EC_MORE)
      {
        uint64_t address;
        memcpy(&address, &id[0], sizeof(address));
        ESP_LOGI(TAG, "Found dallas device 0x%s", format_hex(address).c_str());
        this->found_sensors_.push_back(address);
      }

      for (auto *sensor : sensors_)
      {
        if (sensor->get_index().has_value())
        {
          auto index = *sensor->get_index();
          if (index > found_sensors_.size())
          {
            ESP_LOGW(TAG, "Index %d higher than the number of sensors (%d)", index, found_sensors_.size());
            status_set_error();
            continue;
          }
          auto address = found_sensors_[index];
          ESP_LOGI(TAG, "Sensor index %d has address 0x%s", index, format_hex(address).c_str());
          sensor->set_address(address);
        }

        if (!sensor->setup())
        {
          status_set_error();
        }
      }
    }

    void DallasNgComponent::dump_config()
    {
      ESP_LOGCONFIG(TAG, "DallasComponent:");
      LOG_PIN("  Pin: ", this->pin_);
      LOG_UPDATE_INTERVAL(this);

      if (this->found_sensors_.empty())
      {
        ESP_LOGW(TAG, "  Found no sensors!");
      }
      else
      {
        ESP_LOGD(TAG, "  Found sensors:");
        for (auto &address : this->found_sensors_)
        {
          ESP_LOGD(TAG, "    0x%s", format_hex(address).c_str());
        }
      }

      for (auto *sensor : sensors_)
      {
        LOG_SENSOR("  ", "Device", sensor);
        if (sensor->get_index().has_value())
        {
          ESP_LOGCONFIG(TAG, "    Index %u", *sensor->get_index());
          if (*sensor->get_index() >= this->found_sensors_.size())
          {
            ESP_LOGE(TAG, "Couldn't find sensor by index - not connected. Proceeding without it.");
            continue;
          }
        }
        ESP_LOGCONFIG(TAG, "    Address: %s", sensor->get_address_name().c_str());
        ESP_LOGCONFIG(TAG, "    Resolution: %u", sensor->get_resolution());
      }
    }

    void DallasNgComponent::update()
    {
      status_clear_warning();
      OneWireNg::ErrorCode result = one_wire_->reset();
      if (result != OneWireNg::EC_SUCCESS)
      {
        ESP_LOGE(TAG, "Failed to reset the bus: %d", result);
        status_set_warning();

        for (auto *sensor : sensors_)
        {
          sensor->publish_state(NAN);
        }
        return;
      }

      {
        one_wire_->addressAll();
        one_wire_->writeByte(DSTherm::CMD_CONVERT_T);
      }

      for (auto *sensor : sensors_)
      {
        set_timeout(sensor->get_address_name(), sensor->millis_to_wait_for_conversion(), [this, sensor]
                    {
          float value;
          if (!sensor->try_get_temperature_c(&value)) {
            status_set_warning();
            return;
          }

          sensor->publish_state(value); });
      }
    }

  } // namespace dallas
} // namespace esphome
