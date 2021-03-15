[back to API](api.md)

Version 4.05  
March 2021  
Christopher R. Martin  

# <a name=top></a>lcmap.h

The `lcmap.h` header file and its corresponding `lcmap.c` file establish maps between the strings encountered in configuration files, their `enum` equivalents in C, and human readable text that can be printed by `lc_show_config()`.

- [The map type](#map)
- [Functions](#fn)
- [Maps available](#avail)

## <a name="map"></a> The `lc_map_t` type

The `lc_map_t` variable type is a struct with three members:

| Member | Type           | Description               |
|:------:|:--------------:|---------------------------|
| `value`  | `int`        | Integer or enum value     |
| `config` | `char*` 	  | Signal's mean value       |
| `message`| `char*`      | Maximum sample value      |

The struct is intended to represent a mapping between three equivalent representations of a single configuration parameter's value.  The string that might be expected in a configuration file is rerepresented by `config`.  The integer or `enum` value that is used by the `lconfig.c` core code is represented by `value`.  Finally, `message` is a human-readable string that can be printed by `lc_show_config()`.

Not all configuration parameters need to be represented with such a value map (e.g. `samplehz` is just a number).  For those that do, the possible legal values are represented by an array of `lc_map_t` elements.  The length of the array is determined by reading the elements sequentially until one with a `value` of -1 is found.  This is equivalent to a `\0` in a C-string.

Note that it is entirely legal for these arrays to have multiple entries with the same `value`.  In this way, multiple alternate configuration strings can map to the same value (e.g. `eth` and `ethernet` are equivalent).

## <a name="fn"></a> Functions

### `lcm_get_message()`

```C
const char * lcm_get_message(const lcm_map_t map[], int value);
```
Given an array of map values, this scans the array from the beginning until an element whose `value` is matches the integer specified.  The `message` is returned from the first matching map element.  In the event of an error (e.g. if the value is not found) a NULL string is returned.

This is used by `lc_show_config()` to generate human readable summaries of entries.

### `lcm_get_config()`

```C
const char * lcm_get_config(const lcm_map_t map[], int value);
```
Given an array of map values, this scans the array from the beginning until an element whose `value` is matches the integer specified.  The `config` string is returned from the first matching map element.  In the event of an error (e.g. if the value is not found) a NULL string is returned.

This is used by `lc_write_config()` to build configuration files and data file headers from configuration structs.

### `lcm_get_value()`

```C
int lcm_get_value(const lcm_map_t map[], char config[], int *value);
```
Given an array of map values, this scans the array from the beginning until an element whose `config` string matches the string specified.  The `value` is written to the variable indicated by `value`.  In the event of an error (if the config string is not found), -1 is returned instead.

This is used by `lc_load_config()` to build the configuration struct from the configuration file.

## <a name="avail"></a> Available maps

The following is a list of maps supplied in the `lcmap.h` header:

`lcm_com_channel`  
`lcm_com_parity`  
`lcm_connection`  
`lcm_connection_status`  
`lcm_device`  
`lcm_aosignal`  
`lcm_edge`  
`lcm_ef_signal`  
`lcm_ef_direction`  
`lcm_ef_debounce`  

For detailed information on their contents, please reference the header file directly.
