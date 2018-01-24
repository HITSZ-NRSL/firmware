#include "hw_config.h"
#include "spi_flash.h"
#include "esp8266_upgrade.h"
#include "params_hal.h"
#include "md5_hash.h"

LOCAL struct MD5Context _ctx;
LOCAL struct espconn *upgrade_conn;
LOCAL uint8 *pbuf;
LOCAL os_timer_t upgrade_rev_timer;
LOCAL os_timer_t upgrade_connect_timer;
LOCAL os_timer_t upgrade_timer;
LOCAL uint32 totallength = 0;
LOCAL uint32 sumlength = 0;
LOCAL struct upgrade_param *upgrade;

extern uint8_t down_progress;
extern enum file_type_t filetype;

extern int HAL_PARAMS_Set_Boot_ota_app_size(uint32_t size);
extern int HAL_PARAMS_Set_Boot_def_app_size(uint32_t size);
extern int HAL_PARAMS_Set_Boot_boot_size(uint32_t size);

/******************************************************************************
 * FunctionName : system_upgrade_internal
 * Description  : a
 * Parameters   :
 * Returns      :
 *******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR system_upgrade_internal(struct upgrade_param *upgrade, uint8 *data, uint16 len)
{
    bool ret = false;
    if(data == NULL || len == 0){
        return true;
    }
    upgrade->buffer = (uint8 *)os_zalloc(len + upgrade->extra);

    memcpy(upgrade->buffer, upgrade->save, upgrade->extra);
    memcpy(upgrade->buffer + upgrade->extra, data, len);

    len += upgrade->extra;
    upgrade->extra = len & 0x03;
    len -= upgrade->extra;

    memcpy(upgrade->save, upgrade->buffer + len, upgrade->extra);

    do {
        if (upgrade->fw_bin_addr + len >= (upgrade->fw_bin_sec + upgrade->fw_bin_sec_num) * SPI_FLASH_SEC_SIZE) {
            break;
        }

        DEBUG("%x %x\r\n",upgrade->fw_bin_sec_earse,upgrade->fw_bin_addr);
        /* earse sector, just earse when first enter this zone */
        if (upgrade->fw_bin_sec_earse != (upgrade->fw_bin_addr + len) >> 12) {
            uint16 lentmp = len;
            while( lentmp > SPI_FLASH_SEC_SIZE ){
                upgrade->fw_bin_sec_earse += 1;
                spi_flash_erase_sector(upgrade->fw_bin_sec_earse);
                lentmp -= SPI_FLASH_SEC_SIZE;
            }
            upgrade->fw_bin_sec_earse = (upgrade->fw_bin_addr + len) >> 12;
            spi_flash_erase_sector(upgrade->fw_bin_sec_earse);
            DEBUG("%x\r\n",upgrade->fw_bin_sec_earse);
        }

        if (spi_flash_write(upgrade->fw_bin_addr, (uint32 *)upgrade->buffer, len) != SPI_FLASH_RESULT_OK) {
            break;
        }

        ret = true;
        upgrade->fw_bin_addr += len;
    } while (0);

    free(upgrade->buffer);
    upgrade->buffer = NULL;
    return ret;
}

/******************************************************************************
 * FunctionName : system_upgrade
 * Description  : a
 * Parameters   :
 * Returns      :
 *******************************************************************************/
bool ICACHE_FLASH_ATTR system_upgrade(uint8 *data, uint16 len)
{
    bool ret;

    ret = system_upgrade_internal(upgrade, data, len);
    return ret;
}

/******************************************************************************
 * FunctionName : system_upgrade_init
 * Description  : a
 * Parameters   :
 * Returns      :
 *******************************************************************************/
void ICACHE_FLASH_ATTR system_upgrade_init(void)
{
    if (upgrade == NULL) {
        upgrade = (struct upgrade_param *)os_zalloc(sizeof(struct upgrade_param));
    }

    system_upgrade_flag_set(UPGRADE_FLAG_IDLE);

    if (ONLINE_APP_FILE == filetype){
        upgrade->fw_bin_sec = CACHE_ONLINE_APP_SEC_START;
        upgrade->fw_bin_sec_num = CACHE_ONLINE_APP_SEC_NUM;
    }
#if PLATFORM_NUT == PLATFORM_ID
    else if (BOOT_FILE == filetype){
        upgrade->fw_bin_sec = CACHE_BOOT_SEC_START;
        upgrade->fw_bin_sec_num = CACHE_BOOT_SEC_NUM;
    }
    else {
        upgrade->fw_bin_sec = CACHE_DEFAULT_APP_SEC_START;
        upgrade->fw_bin_sec_num = CACHE_DEFAULT_APP_SEC_NUM;
    }
#endif
    DEBUG("sec=%d  sec_num=%d\r\n", upgrade->fw_bin_sec, upgrade->fw_bin_sec_num);
    upgrade->fw_bin_addr = upgrade->fw_bin_sec * SPI_FLASH_SEC_SIZE;
}

/******************************************************************************
 * FunctionName : system_upgrade_deinit
 * Description  : a
 * Parameters   :
 * Returns      :
 *******************************************************************************/
void ICACHE_FLASH_ATTR system_upgrade_deinit(void)
{
    free(upgrade);
    upgrade = NULL;
}

/******************************************************************************
 * FunctionName : upgrade_disconcb
 * Description  : The connection has been disconnected successfully.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR upgrade_disconcb(void *arg){
    struct espconn *pespconn = arg;

    if (pespconn == NULL) {
        return;
    }

    free(pespconn->proto.tcp);
    pespconn->proto.tcp = NULL;
    free(pespconn);
    pespconn = NULL;
    upgrade_conn = NULL;
}

/******************************************************************************
 * FunctionName : upgrade_datasent
 * Description  : Data has been sent successfully,This means that more data can
 *                be sent.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR upgrade_datasent(void *arg){
    struct espconn *pespconn = arg;

    if (pespconn ->state == ESPCONN_CONNECT) {
    }
}

/******************************************************************************
 * FunctionName : upgrade_deinit
 * Description  : disconnect the connection with the host
 * Parameters   : bin -- server number
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR LOCAL upgrade_deinit(void){
    if (system_upgrade_flag_check() != UPGRADE_FLAG_START) {
        system_upgrade_deinit();
    }
}

//连接服务器超时回调
LOCAL void ICACHE_FLASH_ATTR upgrade_connect_timeout_cb(struct espconn *pespconn){
    struct upgrade_server_info *server;

    DEBUG("upgrade_connect_timeout_cb\r\n");
    if (pespconn == NULL) {
        return;
    }
    server = pespconn->reverse;
    os_timer_disarm(&upgrade_timer);
    system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
    server->upgrade_flag = false;
    if (server->check_cb != NULL) {
        server->check_cb(server);
    }
    system_upgrade_deinit();
    free(pespconn->proto.tcp);
    pespconn->proto.tcp = NULL;
    free(pespconn);
    pespconn = NULL;
    upgrade_conn = NULL;
}

//下载结果检查
LOCAL void ICACHE_FLASH_ATTR upgrade_check(struct upgrade_server_info *server){
    DEBUG("upgrade_check\r\n");
    if (server == NULL) {
        return;
    }

    os_timer_disarm(&upgrade_timer);
    if (system_upgrade_flag_check() != UPGRADE_FLAG_FINISH) {
        totallength = 0;
        sumlength = 0;
        server->upgrade_flag = false;
        system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
    } else {
        server->upgrade_flag = true;
        os_timer_disarm(&upgrade_timer);
    }
    upgrade_deinit();

    if (server->check_cb != NULL) {
        server->check_cb(server);
    }
    espconn_disconnect(upgrade_conn);
}

/******************************************************************************
 * FunctionName : upgrade_download
 * Description  : Processing the upgrade data from the host
 * Parameters   : bin -- server number
 *                pusrdata -- The upgrade data (or NULL when the connection has been closed!)
 *                length -- The length of upgrade data
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR upgrade_download(void *arg, char *pusrdata, unsigned short length){
    char *ptr = NULL;
    char *ptmp2 = NULL;
    char lengthbuffer[32], returncode[4];
    uint8_t md5_calc[16], i = 0;
    char output[64] = {0};
    struct upgrade_server_info *server = (struct upgrade_server_info *)upgrade_conn->reverse;

    //检查返回码
    if (totallength == 0){
        DEBUG("httpdata:%s\r\n", pusrdata);
        ptr = (char *)strstr(pusrdata, "HTTP/1.1 ");
        memset(returncode, 0, sizeof(returncode));
        memcpy(returncode, ptr+9, 3);

        if(strcmp(returncode ,"200")){ //下载失败
            DEBUG("http download return code  error\r\n");
            upgrade_check(server);
            return;
        }
    }

    if (totallength == 0 && (ptr = (char *)strstr(pusrdata, "\r\n\r\n")) != NULL &&
        (ptr = (char *)strstr(pusrdata, "Content-Length")) != NULL) {
        ptr = (char *)strstr(pusrdata, "\r\n\r\n");
        length -= ptr - pusrdata;
        length -= 4;
        totallength += length;
        DEBUG("upgrade file download start.\r\n");
        MD5Init(&_ctx);
        MD5Update(&_ctx, (uint8_t *)ptr + 4, length);
        system_upgrade((uint8_t *)ptr + 4, length);
        ptr = (char *)strstr(pusrdata, "Content-Length: ");
        if (ptr != NULL) {
            ptr += 16;
            ptmp2 = (char *)strstr(ptr, "\r\n");

            if (ptmp2 != NULL) {
                memset(lengthbuffer, 0, sizeof(lengthbuffer));
                memcpy(lengthbuffer, ptr, ptmp2 - ptr);
                sumlength = atoi(lengthbuffer);
                uint32_t  limit_size = 0;
                if (ONLINE_APP_FILE == filetype) {
                    limit_size = CACHE_ONLINE_APP_SEC_NUM * SPI_FLASH_SEC_SIZE;
                }
#if PLATFORM_NUT == PLATFORM_ID
                else if (BOOT_FILE == filetype) {
                    limit_size = CACHE_BOOT_SEC_NUM * SPI_FLASH_SEC_SIZE;
                }
                else if (DEFAULT_APP_FILE == filetype) {
                    limit_size = CACHE_DEFAULT_APP_SEC_NUM * SPI_FLASH_SEC_SIZE;
                }
#endif

                if(sumlength >= limit_size){
                    DEBUG("sumlength failed\r\n");
                    upgrade_check(server);
                    return;
                }
                //获取文件MD5码
                ptr = (char *)strstr(pusrdata, "X-Md5: ");
                if (ptr != NULL) {
                    ptr += 7;
                    ptmp2 = (char *)strstr(ptr, "\r\n");

                    if (ptmp2 != NULL) {
                        memset(server->md5, 0, sizeof(server->md5));
                        memcpy(server->md5, ptr, ptmp2 - ptr);
                    } else {
                        DEBUG("X-Mdt failed\r\n");
                        upgrade_check(server);
                        return;
                    }
                }
            } else {
                DEBUG("sumlength failed\r\n");
                upgrade_check(server);
                return;
            }
        } else {
            upgrade_check(server);
            DEBUG("Content-Length: failed\r\n");
            return;
        }
    } else {
        if(totallength + length > sumlength)
            {length = sumlength - totallength;}
        totallength += length;
        DEBUG("totallen = %d\r\n",totallength);
        MD5Update(&_ctx, (uint8_t *)pusrdata, length);
        system_upgrade((uint8_t *)pusrdata, length);
    }

    if (ONLINE_APP_FILE == filetype) {
        down_progress = totallength*100/sumlength;
    }
#if PLATFORM_NUT == PLATFORM_ID
    else if (BOOT_FILE == filetype) { //下载完毕进度60
        down_progress = totallength*10/sumlength;
    }
    else if (DEFAULT_APP_FILE == filetype) { //下载完毕进度100
        down_progress = 10+totallength*90/sumlength;
    }
#endif

    if ((totallength == sumlength)) {
        DEBUG("upgrade file download finished.\r\n");
        MD5Final(md5_calc, &_ctx);
        memset(output, 0, sizeof(output));
        for(i = 0; i < 16; i++)
        {
            sprintf(output + (i * 2), "%02x", md5_calc[i]);
        }
        DEBUG("md5 = %s\r\n",output);
        DEBUG("server->md5 = %s\r\n",server->md5);
        if(!strcmp(server->md5,output)){
            DEBUG("md5 check ok.\r\n");
            system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
            if (ONLINE_APP_FILE == filetype) {
                HAL_PARAMS_Set_Boot_ota_app_size(sumlength);
            }
#if PLATFORM_NUT == PLATFORM_ID
            else if (BOOT_FILE == filetype) {
                HAL_PARAMS_Set_Boot_boot_size(sumlength);
            }
            else if (DEFAULT_APP_FILE == filetype) {
                HAL_PARAMS_Set_Boot_def_app_size(sumlength);
            }
#endif
            else {
            }
            HAL_PARAMS_Save_Params();
            totallength = 0;
            sumlength = 0;
            upgrade_check(server);
            return;
        }
        DEBUG("md5 check error.\r\n");
        upgrade_check(server);
        return;
    }

    if (upgrade_conn->state != ESPCONN_READ) {
        totallength = 0;
        sumlength = 0;
        os_timer_disarm(&upgrade_rev_timer);
        os_timer_setfn(&upgrade_rev_timer, (os_timer_func_t *)upgrade_check, server);
        os_timer_arm(&upgrade_rev_timer, 10, 0);
    }
}

/******************************************************************************
 * FunctionName : upgrade_connect
 * Description  : client connected with a host successfully
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR upgrade_connect_cb(void *arg){
    struct espconn *pespconn = arg;

    DEBUG("upgrade_connect_cb\r\n");
    os_timer_disarm(&upgrade_connect_timer);

    espconn_regist_disconcb(pespconn, upgrade_disconcb);
    espconn_regist_sentcb(pespconn, upgrade_datasent);

    if (pbuf != NULL) {
        DEBUG("%s\r\n", pbuf);
        espconn_sent(pespconn, pbuf, strlen((char *)pbuf));
    }
}

/******************************************************************************
 * FunctionName : upgrade_connection
 * Description  : connect with a server
 * Parameters   : bin -- server number
 *                url -- the url whitch upgrade files saved
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR upgrade_connect(struct upgrade_server_info *server){
    DEBUG("upgrade_connect\r\n");

    pbuf = server->url;
    espconn_regist_connectcb(upgrade_conn, upgrade_connect_cb);
    espconn_regist_recvcb(upgrade_conn, upgrade_download);

    system_upgrade_init();
    system_upgrade_flag_set(UPGRADE_FLAG_START);

    espconn_connect(upgrade_conn);

    os_timer_disarm(&upgrade_connect_timer);
    os_timer_setfn(&upgrade_connect_timer, (os_timer_func_t *)upgrade_connect_timeout_cb, upgrade_conn);
    os_timer_arm(&upgrade_connect_timer, 5000, 0);
}

/******************************************************************************
 * FunctionName : user_upgrade_init
 * Description  : parameter initialize as a client
 * Parameters   : server -- A point to a server parmer which connected
 * Returns      : none
 *******************************************************************************/
bool ICACHE_FLASH_ATTR system_upgrade_start(struct upgrade_server_info *server){

    if (system_upgrade_flag_check() == UPGRADE_FLAG_START) {
        return false;
    }
    if (server == NULL) {
        DEBUG("server is NULL\r\n");
        return false;
    }
    if (upgrade_conn == NULL) {
        upgrade_conn = (struct espconn *)os_zalloc(sizeof(struct espconn));
    }

    if (upgrade_conn != NULL) {
        upgrade_conn->proto.tcp = NULL;
        upgrade_conn->type = ESPCONN_TCP;
        upgrade_conn->state = ESPCONN_NONE;
        upgrade_conn->reverse = server;

        if (upgrade_conn->proto.tcp == NULL) {
            upgrade_conn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
        }

        if (upgrade_conn->proto.tcp != NULL) {
            upgrade_conn->proto.tcp->local_port = espconn_port();
            upgrade_conn->proto.tcp->remote_port = server->port;
            memcpy(upgrade_conn->proto.tcp->remote_ip, server->ip, 4);
            upgrade_connect(server);

            if (server->check_cb !=  NULL) {
                os_timer_disarm(&upgrade_timer);
                os_timer_setfn(&upgrade_timer, (os_timer_func_t *)upgrade_check, server);
                os_timer_arm(&upgrade_timer, server->check_times, 0);
            }
        }
    }
    return true;
}
