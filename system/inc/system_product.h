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

#ifndef SYSTEM_PRODUCT_H_
#define SYSTEM_PRODUCT_H_

#include <stdint.h>
#include <string.h>
#include "intorobot_macros.h"

#ifndef DEFAULT_PRODUCT_ID
// TODO 添加例子灯 DEFAULT_PRODUCT_ID
#define DEFAULT_PRODUCT_ID
#endif

#ifndef DEFAULT_PRODUCT_SECRET
// TODO 添加例子灯 DEFAULT_PRODUCT_SECRET
#define DEFAULT_PRODUCT_SECRET
#endif

#ifndef DEFAULT_PRODUCT_FIRMWARE_VERSION
// TODO 添加例子灯 DEFAULT_PRODUCT_FIRMWARE_VERSION
#define DEFAULT_PRODUCT_FIRMWARE_VERSION (1)
#endif

typedef enum
{
  PRODUCT_MODE_MASTER = 0,
  PRODUCT_MODE_SLAVE,
} product_mode_t;

typedef struct {
    uint16_t product_firmware_version;     //产品程序版本号
    product_mode_t product_mode;           //产品模式  主模式和从模式
    char product_id[36];                   //产品ID
    char product_secret[36];               //产品密钥
} product_details_t;

class IntoRobotProduct
{
public:
    IntoRobotProduct()
    {
        product_firmware_version = DEFAULT_PRODUCT_FIRMWARE_VERSION;     //产品程序版本号
        product_mode = PRODUCT_MODE_MASTER;                      //产品模式  主模式和从模式
        strcpy(product_id, stringify(DEFAULT_PRODUCT_ID));               //产品ID
        strcpy(product_secret, stringify(DEFAULT_PRODUCT_SECRET));       //产品密钥
    }
    ~IntoRobotProduct(){}

    void set_product_id(char *product_id)
    {
        uint16_t templen;

        if (product_id!=NULL) {
            templen = MIN(sizeof(this->product_id)-1, strlen(product_id));
            memset(this->product_id, 0, sizeof(this->product_id));
            memcpy(this->product_id, product_id, templen);
        }
    }

    void set_product_secret(char *product_secret)
    {
        uint16_t templen;

        if (product_secret!=NULL) {
            templen = MIN(sizeof(this->product_secret)-1, strlen(product_secret));
            memset(this->product_secret, 0, sizeof(this->product_secret));
            memcpy(this->product_secret, product_secret, templen);
        }
    }

    void set_product_firmware_version(uint16_t product_firmware_version)
    {
        this->product_firmware_version = product_firmware_version;
    }

    void set_product_mode(product_mode_t product_mode)
    {
        this->product_mode = product_mode;
    }

    void get_product_details(product_details_t& details)
    {
        details.product_firmware_version = this->product_firmware_version;
        details.product_mode = this->product_mode;
        strcpy(details.product_id, this->product_id);
        strcpy(details.product_secret, this->product_secret);
    }

private:
    uint16_t product_firmware_version;     //产品程序版本号
    product_mode_t product_mode;           //产品模式  主模式和从模式
    char product_id[36];                   //产品ID
    char product_secret[36];               //产品密钥
};

extern IntoRobotProduct &system_product_instance(void);

#endif

