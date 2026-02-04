#include "page_main.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static lv_timer_t * timer;
static lv_obj_t * label1;
static lv_obj_t * battery_label;  // 电量标签

static void btn_demo_click(lv_event_t * e);
static void btn_audio_click(lv_event_t * e);
static void btn_robot_click(lv_event_t * e);
static void btn_file_manager_click(lv_event_t * e);
static void btn_calculator_click(lv_event_t * e);
static void btn_bird_click(lv_event_t * e);
static void btn_apple_click(lv_event_t * e);
static void btn_totp_click(lv_event_t * e);
static void btn_recorder_click(lv_event_t * e);
static void timer_tick(lv_event_t * e);
static void update_battery_info(void);  // 更新电量信息函数

lv_obj_t * page_main() {
	lv_obj_t * screen = lv_obj_create(lv_scr_act());
    //lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(screen, LV_DIR_VER);
    

    label1 = lv_label_create(screen);
	lv_label_set_text(label1, "ciallo lvgl");
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);

    // 创建电量显示标签
    battery_label = lv_label_create(screen);
    lv_label_set_text(battery_label, "电量: --%");
    lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, 20); // 在时间下方20像素处

    // 立即更新一次电量信息
    update_battery_info();

    timer = lv_timer_create(timer_tick, 1000, NULL); // 改为1秒更新一次

    lv_obj_t * btn_robot = lv_btn_create(screen);
    lv_obj_set_size(btn_robot, lv_pct(50), lv_pct(25));
    lv_obj_align(btn_robot, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_robot = lv_label_create(btn_robot);
    lv_label_set_text(btn_label_robot, "robot");
    lv_obj_center(btn_label_robot);
    lv_obj_add_event_cb(btn_robot, btn_robot_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_file_manager = lv_btn_create(screen);
    lv_obj_set_size(btn_file_manager, lv_pct(50), lv_pct(25));
    lv_obj_align(btn_file_manager, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_file_manager = lv_label_create(btn_file_manager);
    lv_label_set_text(btn_label_file_manager, "文件管理器");
    lv_obj_center(btn_label_file_manager);
    lv_obj_add_event_cb(btn_file_manager, btn_file_manager_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_calculator = lv_btn_create(screen);
    lv_obj_set_size(btn_calculator, lv_pct(50), lv_pct(25));
    lv_obj_align(btn_calculator, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_calculator = lv_label_create(btn_calculator);
    lv_label_set_text(btn_label_calculator, "计算器");
    lv_obj_center(btn_label_calculator);
    lv_obj_add_event_cb(btn_calculator, btn_calculator_click, LV_EVENT_CLICKED, NULL);

    
    lv_obj_t * btn_bird = lv_btn_create(screen);
    lv_obj_set_size(btn_bird, lv_pct(50), lv_pct(25));
    lv_obj_align(btn_bird, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_bird = lv_label_create(btn_bird);
    lv_label_set_text(btn_label_bird, "flappy bird");
    lv_obj_center(btn_label_bird);
    lv_obj_add_event_cb(btn_bird, btn_bird_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_totp = lv_btn_create(screen);
    lv_obj_set_size(btn_totp, lv_pct(50), lv_pct(25));
    lv_obj_align(btn_totp, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_totp = lv_label_create(btn_totp);
    lv_label_set_text(btn_label_totp, "身份验证器");
    lv_obj_center(btn_label_totp);
    lv_obj_add_event_cb(btn_totp, btn_totp_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_recorder = lv_btn_create(screen);
    lv_obj_set_size(btn_recorder, lv_pct(50), lv_pct(25));
    lv_obj_align(btn_recorder, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_recorder = lv_label_create(btn_recorder);
    lv_label_set_text(btn_label_recorder, "录音机");
    lv_obj_center(btn_label_recorder);
    lv_obj_add_event_cb(btn_recorder, btn_recorder_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_demo = lv_btn_create(screen);
    lv_obj_set_size(btn_demo, lv_pct(50), lv_pct(25));
    lv_obj_align(btn_demo, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_demo = lv_label_create(btn_demo);
    lv_label_set_text(btn_label_demo, "demo page");
    lv_obj_center(btn_label_demo);
    lv_obj_add_event_cb(btn_demo, btn_demo_click, LV_EVENT_CLICKED, NULL);

    /*
    lv_obj_t * btn_apple = lv_btn_create(screen);
    lv_obj_set_size(btn_apple, lv_pct(50), lv_pct(25));
    lv_obj_align(btn_apple, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_apple = lv_label_create(btn_apple);
    lv_label_set_text(btn_label_apple, "apple");
    lv_obj_center(btn_label_apple);
    lv_obj_add_event_cb(btn_apple, btn_apple_click, LV_EVENT_CLICKED, NULL);
    */
    return screen;
}

static void btn_demo_click(lv_event_t * e)      //static可以防止同名冲突
{	
    page_open(page_demo(), NULL);
}

static void btn_audio_click(lv_event_t * e) // static可以防止同名冲突
{
    page_open(page_audio("/mnt/app/factory/play_test.wav"), NULL);
}

static void btn_robot_click(lv_event_t * e)
{
    switchRobot();
}

static void btn_file_manager_click(lv_event_t * e)
{
    page_open(page_file_manager(), NULL);
}

static void btn_calculator_click(lv_event_t * e)
{
    page_open(page_calculator(), NULL);
}

static void btn_bird_click(lv_event_t * e)
{
    page_open(page_bird(), NULL);
}

static void btn_totp_click(lv_event_t * e)
{
    page_open(page_totp(), NULL);
}

static void btn_apple_click(lv_event_t * e)
{
    page_open(page_apple(), NULL);
}

static void btn_recorder_click(lv_event_t * e)
{
    page_open(page_recorder_create(), NULL);
}

// 辅助函数：读取文件内容到字符串（去除换行符）
static int read_file_to_string(const char *filename, char *buffer, int buffer_size) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return -1;
    }
    
    if (fgets(buffer, buffer_size, fp) == NULL) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    
    // 去除换行符
    buffer[strcspn(buffer, "\n")] = 0;
    
    return 0;
}

// 辅助函数：读取文件内容到整数
static int read_file_to_int(const char *filename, int *value) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return -1;
    }
    
    if (fscanf(fp, "%d", value) != 1) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return 0;
}

// 更新电量信息函数
static void update_battery_info(void) {
    int capacity = 0;
    char status_str[32] = "未知";
    char final_text[64];
    int is_charging = 0;
    
    // 1. 读取电池电量
    if (read_file_to_int("/sys/class/power_supply/battery/capacity", &capacity) != 0) {
        capacity = -1; // 表示读取失败
    }
    
    // 2. 优先读取电池状态文件
    if (read_file_to_string("/sys/class/power_supply/battery/status", status_str, sizeof(status_str)) == 0) {
        // 根据状态字符串确定充电状态
        if (strcmp(status_str, "Charging") == 0) {
            strcpy(status_str, "充电中");
            is_charging = 1;
        } else if (strcmp(status_str, "Discharging") == 0) {
            strcpy(status_str, "放电中");
            is_charging = 0;
        } else if (strcmp(status_str, "Full") == 0) {
            strcpy(status_str, "已充满");
            is_charging = 1;
        } else if (strcmp(status_str, "Not charging") == 0) {
            strcpy(status_str, "未充电");
            is_charging = 0;
        } else {
            // 未知状态，保持原样或显示"未知"
            is_charging = 0;
        }
    } else {
        // 如果状态文件读取失败，尝试读取ac和usb的online状态
        int ac_online = 0;
        int usb_online = 0;
        
        read_file_to_int("/sys/class/power_supply/ac/online", &ac_online);
        read_file_to_int("/sys/class/power_supply/usb/online", &usb_online);
        
        if (ac_online == 1 || usb_online == 1) {
            strcpy(status_str, "充电中");
            is_charging = 1;
        } else {
            strcpy(status_str, "放电中");
            is_charging = 0;
        }
    }
    
    // 3. 组合显示文本
    if (capacity >= 0) {
        if (is_charging) {
            snprintf(final_text, sizeof(final_text), "电量: %d%% (%s)", capacity, status_str);
        } else {
            snprintf(final_text, sizeof(final_text), "电量: %d%% (%s)", capacity, status_str);
        }
    } else {
        // 电量读取失败
        if (is_charging) {
            snprintf(final_text, sizeof(final_text), "电量: --%% (%s)", status_str);
        } else {
            snprintf(final_text, sizeof(final_text), "电量: --%% (%s)", status_str);
        }
    }
    
    // 4. 更新标签文本
    lv_label_set_text(battery_label, final_text);
}

static void timer_tick(lv_event_t * e)
{
    char time_text[24];
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm * tm;
    tm = localtime(&tv.tv_sec);

    lv_snprintf(time_text, sizeof(time_text), "%04d-%02d-%02d %02d:%02d:%02d", 
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);
    lv_label_set_text(label1, time_text);
    
    // 更新电量信息
    update_battery_info();
}