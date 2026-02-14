#include "page_recorder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

static lv_obj_t * scr            = NULL;
static lv_obj_t * btn_record     = NULL;
static lv_obj_t * label_btn      = NULL;
static lv_obj_t * label_status   = NULL;
static lv_obj_t * label_filename = NULL;
static lv_obj_t * label_duration = NULL;
static lv_obj_t * label_space    = NULL;
static lv_obj_t * btn_back       = NULL;
static lv_obj_t * btn_delete     = NULL;

static lv_obj_t * dialog_bg          = NULL;
static lv_obj_t * dialog_box         = NULL;
static lv_obj_t * dialog_label       = NULL;
static lv_obj_t * dialog_btn_confirm = NULL;
static lv_obj_t * dialog_btn_cancel  = NULL;

static int is_recording            = 0;
static int dialog_showing          = 0;
static time_t recording_start_time = 0;
static lv_timer_t * duration_timer = NULL;
static char current_filename[128]  = "";

static void create_ui(void);
static void btn_record_cb(lv_event_t * e);
static void btn_back_cb(lv_event_t * e);
static void btn_delete_cb(lv_event_t * e);
static void btn_confirm_cb(lv_event_t * e);
static void btn_cancel_cb(lv_event_t * e);
static void start_recording(void);
static void stop_recording(void);
static int ensure_rec_dir_exists(void);
static void update_status_display(void);
static void update_space_info(void);
static void update_duration_display(void);
static void duration_timer_cb(lv_timer_t * timer);
static void generate_filename(char * buffer, size_t size);
static void show_delete_dialog(void);
static void hide_delete_dialog(void);
static void delete_all_recordings(void);

lv_obj_t * page_recorder_create(void)
{
    scr = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(scr);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

    is_recording         = 0;
    dialog_showing       = 0;
    recording_start_time = 0;
    duration_timer       = NULL;
    current_filename[0]  = '\0';

    create_ui();
    return scr;
}

static void create_ui(void)
{
    // 存储空间，妈妈再也不用担心我看不到剩余存储空间了（
    label_space = lv_label_create(scr);
    lv_obj_set_width(label_space, lv_pct(90));
    lv_obj_align(label_space, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_align(label_space, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_space, lv_color_hex(0x888888), 0);
    update_space_info();

    // 状态
    label_status = lv_label_create(scr);
    lv_obj_set_width(label_status, lv_pct(90));
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 35);
    lv_label_set_text(label_status, "状态: 空闲");
    lv_obj_set_style_text_align(label_status, LV_TEXT_ALIGN_CENTER, 0);

    // 时长
    label_duration = lv_label_create(scr);
    lv_obj_set_width(label_duration, lv_pct(90));
    lv_obj_align(label_duration, LV_ALIGN_TOP_MID, 0, 58);
    lv_label_set_text(label_duration, "时长: 00:00"); // 其实我想改成滚木的
    lv_obj_set_style_text_align(label_duration, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_duration, lv_color_hex(0x666666), 0);

    // 按钮
    btn_record = lv_btn_create(scr);
    lv_obj_set_size(btn_record, 145, 65);
    lv_obj_align(btn_record, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_record, btn_record_cb, LV_EVENT_CLICKED, NULL);

    label_btn = lv_label_create(btn_record);
    lv_label_set_text(label_btn, "开始录音");
    lv_obj_center(label_btn);

    // 文件名
    label_filename = lv_label_create(scr);
    lv_obj_set_width(label_filename, lv_pct(90));
    lv_obj_align(label_filename, LV_ALIGN_CENTER, 0, 55);
    lv_label_set_text(label_filename, "文件: (未开始)");
    lv_obj_set_style_text_align(label_filename, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_filename, lv_color_hex(0x666666), 0);

    // 返回
    btn_back = lv_btn_create(scr);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t * lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(btn_back, btn_back_cb, LV_EVENT_CLICKED, NULL);

    // 删除按钮
    btn_delete = lv_btn_create(scr);
    lv_obj_set_size(btn_delete, 45, 28);
    lv_obj_align(btn_delete, LV_ALIGN_BOTTOM_RIGHT, -2, 0);

    lv_obj_t * lbl_delete = lv_label_create(btn_delete);
    lv_label_set_text(lbl_delete, "清空");
    lv_obj_center(lbl_delete);
    lv_obj_add_event_cb(btn_delete, btn_delete_cb, LV_EVENT_CLICKED, NULL);

    // 删除的样子
    lv_obj_set_style_bg_color(btn_delete, lv_color_hex(0xFF4444), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_delete, lv_color_white(), 0);

    // 初始化对话框
    dialog_bg  = NULL;
    dialog_box = NULL;

    update_status_display();
}

// 录音按钮回调
static void btn_record_cb(lv_event_t * e)
{
    if(!is_recording) {
        start_recording();
    } else {
        stop_recording();
    }
    update_status_display();
}

// 返回按钮回调
static void btn_back_cb(lv_event_t * e)
{
    if(dialog_showing) {
        hide_delete_dialog();
        return;
    }

    // 如果正在录音，先停下来
    if(is_recording) {
        is_recording = 0;
        lv_label_set_text(label_status, "状态: 正在保存...");
        update_status_display();

        // 停止定时器
        if(duration_timer) {
            lv_timer_del(duration_timer);
            duration_timer = NULL;
        }
        // 重置时长显示
        lv_label_set_text(label_duration, "时长: 00:00");

        // 后台继续录音，气死我了搞了半天都不能让它完整录下来，只能延长一会，不然录不完整
        pid_t pid = fork();
        if(pid == 0) {
            sleep(3);
            system("killall arecord 2>/dev/null");
            _exit(0);
        }
    }

    page_back();
}

// 删除按钮回调 显示确认对话框
static void btn_delete_cb(lv_event_t * e)
{
    show_delete_dialog();
}

// 确认按钮回调
static void btn_confirm_cb(lv_event_t * e)
{
    // 先隐藏对话框
    hide_delete_dialog();

    // 执行删除操作
    delete_all_recordings();
}

// 取消按钮回调
static void btn_cancel_cb(lv_event_t * e)
{
    hide_delete_dialog();
}

// 开始录音
static void start_recording(void)
{
    // 如果对话框正在显示，先隐藏
    if(dialog_showing) {
        hide_delete_dialog();
    }

    // 确保目录存在
    if(!ensure_rec_dir_exists()) {
        lv_label_set_text(label_status, "状态: 创建目录失败");
        return;
    }

    // 生成唯一的文件名
    generate_filename(current_filename, sizeof(current_filename));

    // 构建arecord命令
    char command[512];
    snprintf(command, sizeof(command), "arecord -D hw:0,0 -f S16_LE -r 16000 '%s' &", current_filename);

    // 立即执行录音命令并更新状态
    int ret = system(command);
    if(ret == -1 || ret == 127) {
        lv_label_set_text(label_status, "状态: 启动失败");
        current_filename[0] = '\0';
        return;
    }

    // 记录开始时间并启动时长定时器
    recording_start_time = time(NULL);
    is_recording         = 1;
    lv_label_set_text(label_status, "状态: 录音中...");

    // 创建时长更新定时器（每秒更新一次）
    if(duration_timer) {
        lv_timer_del(duration_timer);
    }
    duration_timer = lv_timer_create(duration_timer_cb, 1000, NULL);
}

// 停止录音 - 界面立即停止，后台继续5秒
static void stop_recording(void)
{
    if(is_recording) {
        // 1. 界面立即反馈停止
        is_recording = 0;
        lv_label_set_text(label_status, "状态: 已保存录音文件");

        // 2. 停止时长定时器并重置显示
        if(duration_timer) {
            lv_timer_del(duration_timer);
            duration_timer = NULL;
        }
        lv_label_set_text(label_duration, "时长: 00:00");

        // 3. 创建子进程处理后台延长录音
        pid_t pid = fork();
        if(pid == 0) {
            // 子进程：等待5秒后终止录音进程
            sleep(5);
            system("killall arecord 2>/dev/null");
            _exit(0);
        }

        // 4. 更新存储空间信息
        update_space_info();
    }
}

// 时长更新定时器回调
static void duration_timer_cb(lv_timer_t * timer)
{
    if(!is_recording || recording_start_time == 0) {
        return;
    }

    update_duration_display();
}

// 更新时长显示
static void update_duration_display(void)
{
    if(!is_recording || recording_start_time == 0) {
        return;
    }

    time_t now    = time(NULL);
    long duration = now - recording_start_time;

    // 格式化为 mm:ss
    int minutes = duration / 60;
    int seconds = duration % 60;

    char duration_text[20];
    snprintf(duration_text, sizeof(duration_text), "时长: %02d:%02d", minutes, seconds);
    lv_label_set_text(label_duration, duration_text);
}

// 显示删除确认对话框
static void show_delete_dialog(void)
{
    if(dialog_showing) return;

    dialog_showing = 1;

    // 如果正在录音，先停下 秋豆麻袋！
    if(is_recording) {
        // 界面立即反馈停止
        is_recording = 0;
        lv_label_set_text(label_status, "状态: 正在保存...");
        update_status_display();

        // 停止时长定时器
        if(duration_timer) {
            lv_timer_del(duration_timer);
            duration_timer = NULL;
        }
        lv_label_set_text(label_duration, "时长: 00:00");

        // 后台继续录音5秒
        pid_t pid = fork();
        if(pid == 0) {
            sleep(5);
            system("killall arecord 2>/dev/null");
            _exit(0);
        }
    }

    // 创建半透明背景（覆盖整个屏幕）
    dialog_bg = lv_obj_create(scr);
    lv_obj_set_size(dialog_bg, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(dialog_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(dialog_bg, LV_OPA_50, 0);
    lv_obj_clear_flag(dialog_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(dialog_bg);

    // 创建对话框框体
    dialog_box = lv_obj_create(dialog_bg);
    lv_obj_set_size(dialog_box, 220, 140);
    lv_obj_align(dialog_box, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(dialog_box, lv_color_white(), 0);
    lv_obj_set_style_radius(dialog_box, 10, 0);
    lv_obj_set_style_pad_all(dialog_box, 15, 0);
    lv_obj_clear_flag(dialog_box, LV_OBJ_FLAG_SCROLLABLE);

    // 创建提示文本
    dialog_label = lv_label_create(dialog_box);
    lv_label_set_text(dialog_label, "确认删除所有录音文件？\n此操作不可恢复！");
    lv_obj_set_width(dialog_label, lv_pct(100));
    lv_obj_align(dialog_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_align(dialog_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(dialog_label, lv_color_hex(0xFF4444), 0);

    // 创建确认按钮
    dialog_btn_confirm = lv_btn_create(dialog_box);
    lv_obj_set_size(dialog_btn_confirm, 80, 35);
    lv_obj_align(dialog_btn_confirm, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_t * lbl_confirm = lv_label_create(dialog_btn_confirm);
    lv_label_set_text(lbl_confirm, "确认");
    lv_obj_center(lbl_confirm);
    lv_obj_set_style_bg_color(dialog_btn_confirm, lv_color_hex(0xFF4444), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_confirm, lv_color_white(), 0);
    lv_obj_add_event_cb(dialog_btn_confirm, btn_confirm_cb, LV_EVENT_CLICKED, NULL);

    // 创建取消按钮
    dialog_btn_cancel = lv_btn_create(dialog_box);
    lv_obj_set_size(dialog_btn_cancel, 80, 35);
    lv_obj_align(dialog_btn_cancel, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t * lbl_cancel = lv_label_create(dialog_btn_cancel);
    lv_label_set_text(lbl_cancel, "取消");
    lv_obj_center(lbl_cancel);
    lv_obj_set_style_bg_color(dialog_btn_cancel, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    lv_obj_add_event_cb(dialog_btn_cancel, btn_cancel_cb, LV_EVENT_CLICKED, NULL);
}

// 隐藏删除对话框
static void hide_delete_dialog(void)
{
    if(!dialog_showing) return;

    dialog_showing = 0;

    if(dialog_bg) {
        lv_obj_del(dialog_bg);
        dialog_bg          = NULL;
        dialog_box         = NULL;
        dialog_label       = NULL;
        dialog_btn_confirm = NULL;
        dialog_btn_cancel  = NULL;
    }
}

// 删除所有录音文件
static void delete_all_recordings(void)
{
    // 显示删除提示
    lv_label_set_text(label_status, "状态: 正在删除录音文件...");

    // 执行删除命令
    int ret = system("rm -f /mnt/UDISK/rec/* 2>/dev/null");

    if(ret == 0) {
        lv_label_set_text(label_status, "状态: 录音文件已清空");
        current_filename[0] = '\0';
        lv_label_set_text(label_filename, "文件: (未开始)");
    } else {
        lv_label_set_text(label_status, "状态: 删除失败");
    }

    // 更新存储空间信息
    update_space_info();
}

// 确保录音目录存在
static int ensure_rec_dir_exists(void)
{
    const char * rec_dir = "/mnt/UDISK/rec";

    // 使用mkdir -p创建目录（如果不存在）
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", rec_dir);

    if(system(cmd) != 0) {
        return 0;
    }

    return 1;
}

// 更新存储空间信息 - 格式：已用/总共 MB (百分比%)
static void update_space_info(void)
{
    char cmd[256];
    char buffer[128];
    FILE * fp;

    // 获取/mnt/UDISK分区的总空间和可用空间
    snprintf(cmd, sizeof(cmd), "df -k /mnt/UDISK | tail -1 | awk '{print $2,$3}'");
    fp = popen(cmd, "r");

    if(fp && fgets(buffer, sizeof(buffer), fp)) {
        int total_kb, used_kb;
        sscanf(buffer, "%d %d", &total_kb, &used_kb);

        // 转换为MB并保留一位小数
        float total_mb = total_kb / 1024.0;
        float used_mb  = used_kb / 1024.0;
        int percent    = (int)((used_mb / total_mb) * 100);

        // 格式：存储：已用/总共 MB (百分比%)
        char info[64];
        snprintf(info, sizeof(info), "存储: %.1f/%.1f MB (%d%%)", used_mb, total_mb, percent);

        lv_label_set_text(label_space, info);

        pclose(fp);
    } else {
        lv_label_set_text(label_space, "存储: --/-- MB (--%)");
    }
}

// 生成文件名：年份取后两位
static void generate_filename(char * buffer, size_t size)
{
    if(!buffer || size < 20) return;

    time_t now          = time(NULL);
    struct tm * tm_info = localtime(&now);

    // 格式: 两位年+月+日_时+分+秒.wav
    snprintf(buffer, size, "/mnt/UDISK/rec/%02d%02d%02d_%02d%02d%02d.wav", tm_info->tm_year % 100, tm_info->tm_mon + 1,
             tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
}

// 更新状态显示
static void update_status_display(void)
{
    // 更新按钮文字和颜色
    if(is_recording) {
        lv_label_set_text(label_btn, "停止录音");
        lv_obj_set_style_bg_color(btn_record, lv_color_hex(0xFF5555), LV_PART_MAIN);
    } else {
        lv_label_set_text(label_btn, "开始录音");
        lv_obj_set_style_bg_color(btn_record, lv_color_hex(0x4CAF50), LV_PART_MAIN);
    }

    // 更新文件名显示
    if(current_filename[0] != '\0') {
        const char * filename_only = strrchr(current_filename, '/');
        if(filename_only) {
            char display_text[150];
            snprintf(display_text, sizeof(display_text), "文件: %s", filename_only + 1);
            lv_label_set_text(label_filename, display_text);
        }
    }
}

// 你好，代码写的这么烂你居然都看完了，不要喷啦(>﹏<)