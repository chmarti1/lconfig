
/*
  This file is part of the LCONFIG laboratory configuration system.

    LCONFIG is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LCONFIG is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LCONFIG.  If not, see <https://www.gnu.org/licenses/>.

    Authored by C.Martin crm28@psu.edu
*/

#include "lcmap.h"



/* LCM_GET_MESSAGE
The MAP is an array of lcm_map_t structs mapping an integer/enum VALUE
to a human-readable string describing the state of the configuration.
*/
const char * lcm_get_message(const lcm_map_t map[], int value){
    int ii;
    // Loop through the map values; examine no more than 256 values!
    for(ii=0; ii<=LCM_MAX_VALUE; ii++){
        // If this is the end of the map, 
        if(map[ii].value<0)
            return lcm_errors[0];
        else if(map[ii].value == value){
            if(map[ii].message)
                return map[ii].message;
            else
                return lcm_errors[0];
        }
    }
    return lcm_errors[1];
}

/* LCM_GET_CONFIG
The MAP is an array of lcm_map_t structs mapping an integer/enum VALUE
to a configuration string value
*/
const char * lcm_get_config(const lcm_map_t map[], int value){
    int ii;
    // Loop through the map values; examine no more than 256 values!
    for(ii=0; ii<=LCM_MAX_VALUE; ii++){
        // If this is the end of the map, 
        if(map[ii].value<0)
            return lcm_errors[0];
        else if(map[ii].value == value){
            if(map[ii].config)
                return map[ii].config;
            else
                return lcm_errors[0];
        }
    }
    return lcm_errors[1];
}

/* LCM_GET_VALUE
Given a configuration string, return the enumerated value from the map.  If the
configuration string is not found, return -1.  On success, the enumerated map 
value is returned in VALUE
*/
int lcm_get_value(const lcm_map_t map[], char config[], int *value){
    int ii;
    // Loop through the map values; examine no more than 256 values!
    for(ii=0; ii<=LCM_MAX_VALUE; ii++){
        // If this is the end of the map, 
        if(map[ii].value<0)
            return -1;
        else if(map[ii].config && strcmp(map[ii].config,config) == 0){
            *value = map[ii].value;
            return 0;
        }
    }
    return -1;
}
