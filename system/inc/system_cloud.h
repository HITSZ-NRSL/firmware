/**
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
*/

#ifndef SYSTEM_CLOUD_H_
#define SYSTEM_CLOUD_H_

#include "intorobot_config.h"

#ifdef configNO_CLOUD
#define CLOUD_FN(x,y) (y)
#else
#define CLOUD_FN(x,y) (x)
#endif


#include "static_assert.h"
#include "wiring_string.h"
#include <string.h>
#include <time.h>
#include <stdint.h>

#ifndef configNO_CLOUD

#define MAX_CALLBACK_NUM    32

typedef void (*pCallBack)(uint8_t*, uint32_t);

typedef enum
{
    API_VERSION_V1 = 1,
    API_VERSION_V2 = 2
} api_version_t;

class WidgetBaseClass
{
public:
    virtual void begin(void (*UserCallBack)(void)) {}
    virtual void widgetBaseCallBack(uint8_t *payload, uint32_t len) {}
};

struct CallBackNode
{
    void (*callback)(uint8_t*, uint32_t);
    uint8_t qos;
    const char *topic;
    const char *device_id;
    api_version_t version;
};

struct WidgetCallBackNode
{
    WidgetBaseClass *pWidgetBase;
    uint8_t qos;
    const char *topic;
    const char *device_id;
    api_version_t version;
};

struct CallBackList
{
    struct CallBackNode callback_node[MAX_CALLBACK_NUM];
    struct WidgetCallBackNode widget_callback_node[MAX_CALLBACK_NUM];
    int total_callbacks;
    int total_wcallbacks;
};

#define CLOUD_DEBUG_BUFFER_SIZE 128

struct CloudDebugBuffer
{
    unsigned char buffer[CLOUD_DEBUG_BUFFER_SIZE];
    volatile unsigned int head;
    volatile unsigned int tail;
};

#ifdef __cplusplus
extern "C" {
#endif

void intorobot_send_upgrade_progress(uint8_t progress);
void intorobot_cloud_init(void);
bool intorobot_publish(api_version_t version, const char* topic, uint8_t* payload, unsigned int plength, uint8_t qos, uint8_t retained);
bool intorobot_subscribe(api_version_t version, const char* topic, const char *device_id, void (*callback)(uint8_t*, uint32_t), uint8_t qos);
bool intorobot_widget_subscribe(api_version_t version, const char* topic, const char *device_id, WidgetBaseClass *pWidgetBase, uint8_t qos);
bool intorobot_unsubscribe(api_version_t version, const char *topic, const char *device_id);
size_t intorobot_debug_info_write(uint8_t byte);
int intorobot_debug_info_read(void);
int intorobot_debug_info_available(void);

bool intorobot_cloud_flag_connected(void);
void intorobot_cloud_disconnect(void);
int intorobot_cloud_connect(void);
int intorobot_cloud_handle(void);

void intorobot_cloud_flag_connect(void);
void intorobot_cloud_flag_disconnect(void);
bool intorobot_cloud_flag_auto_connect();


#ifdef __cplusplus
}
#endif

#endif

String intorobot_deviceID(void);
void intorobot_process(void);
bool intorobot_sync_time(void);
bool intorobot_device_register(void);
bool intorobot_device_activate(void);
bool intorobot_get_version(String &body);
void cloud_disconnect(bool closeSocket=true);


#endif	/* SYSTEM_CLOUD_H_ */
