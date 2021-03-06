// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/iot/az_iot_pnp_client.h>

#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

static const az_span iot_hub_property_desired = AZ_SPAN_LITERAL_FROM_STR("desired");
static const az_span iot_hub_property_desired_version = AZ_SPAN_LITERAL_FROM_STR("$version");
static const az_span property_response_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static const az_span property_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static const az_span property_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static const az_span property_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static const az_span component_property_label_name = AZ_SPAN_LITERAL_FROM_STR("__t");
static const az_span component_property_label_value = AZ_SPAN_LITERAL_FROM_STR("c");

AZ_NODISCARD az_result az_iot_pnp_client_property_patch_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  return az_iot_hub_client_twin_patch_get_publish_topic(
      &(client->_internal.iot_hub_client),
      request_id,
      mqtt_topic,
      mqtt_topic_size,
      out_mqtt_topic_length);
}

AZ_NODISCARD az_result az_iot_pnp_client_property_document_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  return az_iot_hub_client_twin_document_get_publish_topic(
      &(client->_internal.iot_hub_client),
      request_id,
      mqtt_topic,
      mqtt_topic_size,
      out_mqtt_topic_length);
}

AZ_NODISCARD az_result az_iot_pnp_client_property_parse_received_topic(
    az_iot_pnp_client const* client,
    az_span received_topic,
    az_iot_pnp_client_property_response* out_response)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _az_PRECONDITION_NOT_NULL(out_response);

  az_iot_hub_client_twin_response hub_twin_response;
  _az_RETURN_IF_FAILED(az_iot_hub_client_twin_parse_received_topic(
      &client->_internal.iot_hub_client, received_topic, &hub_twin_response));

  out_response->request_id = hub_twin_response.request_id;
  out_response->response_type
      = (az_iot_pnp_client_property_response_type)hub_twin_response.response_type;
  out_response->status = hub_twin_response.status;
  out_response->version = hub_twin_response.version;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_property_builder_begin_component(
    az_iot_pnp_client const* client,
    az_json_writer* ref_json_writer,
    az_span component_name)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_json_writer);
  _az_PRECONDITION_VALID_SPAN(component_name, 1, false);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(ref_json_writer, component_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(ref_json_writer, component_property_label_name));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_string(ref_json_writer, component_property_label_value));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_property_builder_end_component(
    az_iot_pnp_client const* client,
    az_json_writer* ref_json_writer)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_json_writer);

  (void)client;

  return az_json_writer_append_end_object(ref_json_writer);
}

AZ_NODISCARD az_result az_iot_pnp_client_property_builder_begin_reported_status(
    az_iot_pnp_client const* client,
    az_json_writer* ref_json_writer,
    az_span property_name,
    int32_t ack_code,
    int32_t ack_version,
    az_span ack_description)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_json_writer);
  _az_PRECONDITION_VALID_SPAN(property_name, 1, false);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(ref_json_writer, property_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(ref_json_writer, property_ack_code_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_int32(ref_json_writer, ack_code));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(ref_json_writer, property_ack_version_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_int32(ref_json_writer, ack_version));

  if (az_span_size(ack_description) != 0)
  {
    _az_RETURN_IF_FAILED(
        az_json_writer_append_property_name(ref_json_writer, property_ack_description_name));
    _az_RETURN_IF_FAILED(az_json_writer_append_string(ref_json_writer, ack_description));
  }

  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(ref_json_writer, property_response_value_name));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_property_builder_end_reported_status(
    az_iot_pnp_client const* client,
    az_json_writer* ref_json_writer)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_json_writer);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(ref_json_writer));

  return AZ_OK;
}

// Move reader to the value of property name
static az_result json_child_token_move(az_json_reader* ref_jr, az_span property_name)
{
  do
  {
    if ((ref_jr->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
        && az_json_token_is_text_equal(&(ref_jr->token), property_name))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_jr));

      return AZ_OK;
    }
    else if (ref_jr->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_result_failed(az_json_reader_skip_children(ref_jr)))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (ref_jr->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
  } while (az_result_succeeded(az_json_reader_next_token(ref_jr)));

  return AZ_ERROR_ITEM_NOT_FOUND;
}

// Check if the component name is in the model
static bool is_component_in_model(
    az_iot_pnp_client const* client,
    az_json_token const* component_name,
    az_span* out_component_name)
{
  int32_t index = 0;

  while (index < client->_internal.options.component_names_length)
  {
    if (az_json_token_is_text_equal(
            component_name, client->_internal.options.component_names[index]))
    {
      *out_component_name = client->_internal.options.component_names[index];
      return true;
    }

    index++;
  }

  return false;
}

AZ_NODISCARD az_result az_iot_pnp_client_property_get_property_version(
    az_iot_pnp_client const* client,
    az_json_reader* ref_json_reader,
    az_iot_pnp_client_property_response_type response_type,
    int32_t* out_version)
{
  _az_PRECONDITION_NOT_NULL(client);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

  if (ref_json_reader->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

  if (response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET)
  {
    _az_RETURN_IF_FAILED(json_child_token_move(ref_json_reader, iot_hub_property_desired));
    _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
  }

  _az_RETURN_IF_FAILED(json_child_token_move(ref_json_reader, iot_hub_property_desired_version));
  _az_RETURN_IF_FAILED(az_json_token_get_int32(&ref_json_reader->token, out_version));

  return AZ_OK;
}

static az_result check_if_skippable(
    az_json_reader* jr,
    az_iot_pnp_client_property_response_type response_type)
{
  // First time move
  if (jr->_internal.bit_stack._internal.current_depth == 0)
  {
    _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

    if (jr->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }

    _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

    if (response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET)
    {
      _az_RETURN_IF_FAILED(json_child_token_move(jr, iot_hub_property_desired));
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
    }
  }
  while (true)
  {
    // Within the "root" or "component name" section
    if ((response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES
         && jr->_internal.bit_stack._internal.current_depth == 1)
        || (response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET
            && jr->_internal.bit_stack._internal.current_depth == 2))
    {
      if ((az_json_token_is_text_equal(&jr->token, iot_hub_property_desired_version)))
      {
        // Skip version property name and property value
        _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
        _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

        continue;
      }
      else
      {
        return AZ_OK;
      }
    }
    // Within the property value section
    else if (
        (response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES
         && jr->_internal.bit_stack._internal.current_depth == 2)
        || (response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET
            && jr->_internal.bit_stack._internal.current_depth == 3))
    {
      if (az_json_token_is_text_equal(&jr->token, component_property_label_name))
      {
        // Skip label property name and property value
        _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
        _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

        continue;
      }
      else
      {
        return AZ_OK;
      }
    }
    else
    {
      return AZ_OK;
    }
  }
}

/*
Assuming a JSON of either the below types

AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES:

{
  //ROOT COMPONENT or COMPONENT NAME section
  "component_one": {
    //PROPERTY VALUE section
    "prop_one": 1,
    "prop_two": "string"
  },
  "component_two": {
    "prop_three": 45,
    "prop_four": "string"
  },
  "not_component": 42,
  "$version": 5
}

AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET:

{
  "desired": {
    //ROOT COMPONENT or COMPONENT NAME section
    "component_one": {
        //PROPERTY VALUE section
        "prop_one": 1,
        "prop_two": "string"
    },
    "component_two": {
        "prop_three": 45,
        "prop_four": "string"
    },
    "not_component": 42,
    "$version": 5
  },
  "reported": {
      "manufacturer": "Sample-Manufacturer",
      "model": "pnp-sample-Model-123",
      "swVersion": "1.0.0.0",
      "osName": "Contoso"
  }
}

*/
AZ_NODISCARD az_result az_iot_pnp_client_property_get_next_component_property(
    az_iot_pnp_client const* client,
    az_json_reader* ref_json_reader,
    az_iot_pnp_client_property_response_type response_type,
    az_span* out_component_name,
    az_json_reader* out_property_name_and_value)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_json_reader);
  _az_PRECONDITION_NOT_NULL(out_component_name);
  _az_PRECONDITION_NOT_NULL(out_property_name_and_value);

  (void)client;

  while (true)
  {
    _az_RETURN_IF_FAILED(check_if_skippable(ref_json_reader, response_type));

    if (ref_json_reader->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      // At the end of the "root component" or "component name". Done parsing.
      if ((response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES
           && ref_json_reader->_internal.bit_stack._internal.current_depth == 0)
          || (response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET
              && ref_json_reader->_internal.bit_stack._internal.current_depth == 1))
      {
        return AZ_ERROR_IOT_END_OF_PROPERTIES;
      }

      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      // Continue loop if at the end of the component
      continue;
    }

    break;
  }

  // Check if within the "root component" or "component name" section
  if ((response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_DESIRED_PROPERTIES
       && ref_json_reader->_internal.bit_stack._internal.current_depth == 1)
      || (response_type == AZ_IOT_PNP_CLIENT_PROPERTY_RESPONSE_TYPE_GET
          && ref_json_reader->_internal.bit_stack._internal.current_depth == 2))
  {
    if (is_component_in_model(client, &ref_json_reader->token, out_component_name))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

      if (ref_json_reader->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      _az_RETURN_IF_FAILED(check_if_skippable(ref_json_reader, response_type));
    }
    else
    {
      *out_component_name = AZ_SPAN_EMPTY;
    }
  }

  *out_property_name_and_value = *ref_json_reader;

  // Skip the property value array (if applicable) and move to next token
  _az_RETURN_IF_FAILED(az_json_reader_skip_children(ref_json_reader));
  _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

  return AZ_OK;
}
