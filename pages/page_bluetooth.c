#include "page_bluetooth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>

// 蓝牙页面结构体
typedef struct
{
    lv_obj_t * screen;       // 屏幕对象
    lv_obj_t * switch_obj;   // 开关对象
    lv_obj_t * table;        // 设备列表表格
    lv_obj_t * status_label; // 状态标签
    lv_obj_t * scan_btn;     // 扫描按钮
    lv_obj_t * back_btn;     // 返回按钮
    lv_obj_t * test_btn;     // 测试按钮

    // 设备列表
    char devices[50][64]; // 设备名称
    char macs[50][18];    // MAC地址
    int device_count;     // 设备数量

    // 状态
    int is_scanning;         // 是否正在扫描
    int bt_initialized;      // 蓝牙是否已初始化
    int init_countdown;      // 初始化倒计时
    lv_timer_t * init_timer; // 初始化定时器
    lv_timer_t * scan_timer; // 扫描超时定时器

    // 当前连接的设备
    char connected_mac[18]; // 当前已连接设备的MAC地址
    int is_playing;         // 是否正在播放测试音频
} bluetooth_page_t;

static bluetooth_page_t bp = {0};

// 函数声明
static void back_click(lv_event_t * e);
static void switch_event(lv_event_t * e);
static void scan_click(lv_event_t * e);
static void table_event(lv_event_t * e);
static void test_click(lv_event_t * e);
static void run_bt_test(void);
static void init_timeout_cb(lv_timer_t * timer);
static void start_bluetoothd(void);
static void stop_bluetoothd(void);
static void start_scan(void);
static void stop_scan(void);
static void scan_timeout_cb(lv_timer_t * timer);
static void parse_devices_from_output(const char * output);
static void add_device_to_list(const char * mac, const char * name);
static void clear_device_list(void);
static void connect_device(const char * mac);
static void execute_command(const char * cmd, char * output, int max_len);
static void update_status(const char * status);
static int check_bt_test_running(void);

// 检查bt_test是否运行
static int check_bt_test_running(void)
{
    FILE * fp = popen("ps | grep bt_test | grep -v grep", "r");
    if(!fp) return 0;
    char buf[128];
    int running = (fgets(buf, sizeof(buf), fp) != NULL);
    pclose(fp);
    return running;
}

// 运行bt_test初始化蓝牙
static void run_bt_test(void)
{
    if(check_bt_test_running()) {
        printf("[BT] bt_test is already running\n");
        bp.bt_initialized = 1;
        update_status("BT is On");
        return;
    }

    printf("[BT] Starting bt_test...\n");
    update_status("Starting BT...");

    pid_t pid = fork();
    if(pid == 0) {
        execlp("bt_test", "bt_test", NULL);
        perror("exec bt_test failed");
        exit(1);
    } else if(pid > 0) {
        printf("[BT] bt_test started with PID %d\n", pid);
        bp.init_countdown = 10;
        if(bp.init_timer == NULL) {
            bp.init_timer = lv_timer_create(init_timeout_cb, 1000, NULL);
        }
    } else {
        perror("fork failed");
        update_status("BT init failed");
    }
}

// 初始化倒计时回调
static void init_timeout_cb(lv_timer_t * timer)
{
    bp.init_countdown--;
    char status[32];
    snprintf(status, sizeof(status), "BT init...%d", bp.init_countdown);
    update_status(status);

    if(bp.init_countdown <= 0) {
        lv_timer_del(bp.init_timer);
        bp.init_timer     = NULL;
        bp.bt_initialized = 1;
        update_status("BT is On");

        // 如果开关已打开，启动bluetoothd
        if(lv_obj_has_state(bp.switch_obj, LV_STATE_CHECKED)) {
            start_bluetoothd();
        }
    }
}

// 启动bluetoothd服务
static void start_bluetoothd(void)
{
    printf("[BT] Starting bluetoothd...\n");
    system("bluetoothd &");
    sleep(1); // 等待启动
    update_status("on");
}

// 停止bluetoothd和bluealsa（但不停止bt_test）
static void stop_bluetoothd(void)
{
    printf("[BT] Stopping bluetooth services...\n");
    system("killall bluetoothd 2>/dev/null");
    system("killall bluealsa 2>/dev/null");
    sleep(1);
}

// 创建蓝牙页面
lv_obj_t * page_bluetooth(void)
{
    if(bp.screen) {
        lv_obj_del(bp.screen);
    }

    memset(&bp, 0, sizeof(bp));
    bp.connected_mac[0] = '\0';
    bp.is_playing       = 0;

    bp.screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(bp.screen);
    lv_obj_set_size(bp.screen, 240, 240);
    lv_obj_set_style_bg_color(bp.screen, lv_color_white(), 0);

    // 返回按钮（左下角）
    bp.back_btn = lv_btn_create(bp.screen);
    lv_obj_set_size(bp.back_btn, lv_pct(25), lv_pct(12));
    lv_obj_align(bp.back_btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t * back_label = lv_label_create(bp.back_btn);
    lv_label_set_text(back_label, "back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(bp.back_btn, back_click, LV_EVENT_CLICKED, NULL);

    // 扫描按钮（右下角）
    bp.scan_btn = lv_btn_create(bp.screen);
    lv_obj_set_size(bp.scan_btn, lv_pct(25), lv_pct(12));
    lv_obj_align(bp.scan_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_t * scan_label = lv_label_create(bp.scan_btn);
    lv_label_set_text(scan_label, "scan");
    lv_obj_center(scan_label);
    lv_obj_add_event_cb(bp.scan_btn, scan_click, LV_EVENT_CLICKED, NULL);

    // 测试按钮（底部中间）
    bp.test_btn = lv_btn_create(bp.screen);
    lv_obj_set_size(bp.test_btn, lv_pct(25), lv_pct(12));
    lv_obj_align(bp.test_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_t * test_label = lv_label_create(bp.test_btn);
    lv_label_set_text(test_label, "test");
    lv_obj_center(test_label);
    lv_obj_add_event_cb(bp.test_btn, test_click, LV_EVENT_CLICKED, NULL);

    // 开关（左上角）
    bp.switch_obj = lv_switch_create(bp.screen);
    lv_obj_set_size(bp.switch_obj, 50, 28);
    lv_obj_align(bp.switch_obj, LV_ALIGN_TOP_LEFT, 2, 2);
    lv_obj_add_event_cb(bp.switch_obj, switch_event, LV_EVENT_VALUE_CHANGED, NULL);

    // 状态标签
    bp.status_label = lv_label_create(bp.screen);
    lv_label_set_text(bp.status_label, "off");
    lv_obj_set_size(bp.status_label, 180, 30);
    lv_obj_align(bp.status_label, LV_ALIGN_TOP_RIGHT, -2, 2);
    lv_obj_set_style_text_align(bp.status_label, LV_TEXT_ALIGN_RIGHT, 0);

    // 设备列表（表格）
    bp.table = lv_table_create(bp.screen);
    lv_obj_set_size(bp.table, 220, 150);
    lv_obj_align(bp.table, LV_ALIGN_TOP_MID, 0, 40);
    lv_table_set_col_width(bp.table, 0, 220);
    lv_table_set_col_cnt(bp.table, 1);
    lv_obj_add_event_cb(bp.table, table_event, LV_EVENT_CLICKED, NULL);
    lv_obj_set_scroll_dir(bp.table, LV_DIR_VER);
    lv_table_set_row_cnt(bp.table, 0);

    return bp.screen;
}

// 返回按钮点击
static void back_click(lv_event_t * e)
{
    // 停止可能的测试播放
    if(bp.is_playing) {
        system("killall aplay 2>/dev/null");
    }

    // 停止扫描和蓝牙服务，但保留bt_test
    stop_scan();
    stop_bluetoothd();

    // 销毁定时器
    if(bp.scan_timer) {
        lv_timer_del(bp.scan_timer);
        bp.scan_timer = NULL;
    }
    if(bp.init_timer) {
        lv_timer_del(bp.init_timer);
        bp.init_timer = NULL;
    }

    page_back();
}

// 开关事件
static void switch_event(lv_event_t * e)
{
    if(lv_obj_has_state(bp.switch_obj, LV_STATE_CHECKED)) {
        // 打开蓝牙
        if(!bp.bt_initialized) {
            run_bt_test(); // 启动bt_test并等待初始化
        } else {
            start_bluetoothd();
            update_status("on");
        }
    } else {
        // 关闭蓝牙服务
        stop_scan(); // 停止可能的扫描
        stop_bluetoothd();
        update_status("off");
        clear_device_list();
        lv_table_set_row_cnt(bp.table, 0);
        // 清空已连接设备记录
        bp.connected_mac[0] = '\0';
        bp.is_playing       = 0;
        // 恢复测试按钮文字
        lv_obj_t * label = lv_obj_get_child(bp.test_btn, 0);
        lv_label_set_text(label, "test");
    }
}

// 扫描按钮点击
static void scan_click(lv_event_t * e)
{
    if(!lv_obj_has_state(bp.switch_obj, LV_STATE_CHECKED)) {
        // 自动打开开关
        lv_obj_add_state(bp.switch_obj, LV_STATE_CHECKED);
        lv_event_send(bp.switch_obj, LV_EVENT_VALUE_CHANGED, NULL);
        // 等待蓝牙准备好
        sleep(2);
    }

    if(!bp.bt_initialized) {
        update_status("BT not ready");
        return;
    }

    // 如果正在扫描，先停止之前的扫描
    if(bp.is_scanning) {
        stop_scan();
    }

    start_scan();
}

// 开始扫描
static void start_scan(void)
{
    if(!bp.bt_initialized) {
        update_status("BT not ready");
        return;
    }

    update_status("scanning...");
    bp.is_scanning = 1;

    // 清除之前的设备列表
    clear_device_list();
    lv_table_set_row_cnt(bp.table, 0);

    // 使用timeout命令启动扫描，10秒后自动停止
    system("timeout 10 bluetoothctl scan on > /dev/null 2>&1 &");

    // 设置扫描超时定时器（10秒后自动结束并获取设备列表）
    if(bp.scan_timer == NULL) {
        bp.scan_timer = lv_timer_create(scan_timeout_cb, 10000, NULL);
    } else {
        lv_timer_reset(bp.scan_timer);
    }
}

// 停止扫描
static void stop_scan(void)
{
    if(!bp.is_scanning) return;

    // 杀死所有bluetoothctl扫描进程
    system("pkill -f 'bluetoothctl scan on' 2>/dev/null");
    bp.is_scanning = 0;

    if(bp.scan_timer) {
        lv_timer_del(bp.scan_timer);
        bp.scan_timer = NULL;
    }
}

// 扫描超时回调
static void scan_timeout_cb(lv_timer_t * timer)
{
    // 扫描超时，此时timeout应该已经终止了bluetoothctl进程
    bp.is_scanning = 0;
    lv_timer_del(bp.scan_timer);
    bp.scan_timer = NULL;

    update_status("获取设备列表...");

    // 获取设备列表
    char output[4096] = {0};
    execute_command("bluetoothctl devices", output, sizeof(output));
    parse_devices_from_output(output);

    char status[32];
    snprintf(status, sizeof(status), "found: %d", bp.device_count);
    update_status(status);
}

// 解析 bluetoothctl devices 输出
static void parse_devices_from_output(const char * output)
{
    clear_device_list();

    const char * line = output;
    char line_buf[256];

    while(line && *line) {
        const char * next = strchr(line, '\n');
        if(next) {
            int len = next - line;
            if(len < (int)sizeof(line_buf)) {
                strncpy(line_buf, line, len);
                line_buf[len] = '\0';
            } else {
                line = next + 1;
                continue;
            }
            line = next + 1;
        } else {
            strncpy(line_buf, line, sizeof(line_buf) - 1);
            line_buf[sizeof(line_buf) - 1] = '\0';
            line                           = NULL;
        }

        if(strncmp(line_buf, "Device ", 7) == 0) {
            char mac[18]   = {0};
            char name[64]  = {0};
            const char * p = line_buf + 7;
            int i;
            for(i = 0; i < 17 && p[i] && p[i] != ' '; i++) {
                mac[i] = p[i];
            }
            mac[i] = '\0';
            p += i;
            while(*p == ' ') p++;
            strncpy(name, p, 63);
            name[63]  = '\0';
            char * nl = strchr(name, '\n');
            if(nl) *nl = '\0';

            if(strlen(mac) == 17) {
                add_device_to_list(mac, name);
                printf("[BT] Found: %s - %s\n", mac, name);
            }
        }
    }
}

// 添加设备到列表
static void add_device_to_list(const char * mac, const char * name)
{
    if(bp.device_count >= 50) return;

    strcpy(bp.macs[bp.device_count], mac);
    strcpy(bp.devices[bp.device_count], name);

    lv_table_set_cell_value(bp.table, bp.device_count, 0, name);
    lv_table_set_row_cnt(bp.table, bp.device_count + 1);

    bp.device_count++;
}

// 清空设备列表
static void clear_device_list(void)
{
    bp.device_count = 0;
    lv_table_set_row_cnt(bp.table, 0);
}

// 表格点击事件
static void table_event(lv_event_t * e)
{
    if(bp.is_scanning) return;

    uint16_t row, col;
    lv_table_get_selected_cell(bp.table, &row, &col);
    if(row < 0 || row >= bp.device_count) return;

    const char * mac = bp.macs[row];
    printf("[BT] Connecting to %s (%s)\n", bp.devices[row], mac);

    // 清空列表（按要求）
    clear_device_list();
    lv_table_set_row_cnt(bp.table, 0);

    connect_device(mac);
}

// 连接设备（改进版）
static void connect_device(const char * mac)
{
    update_status("pairing...");
    char cmd[128];
    char output[1024] = {0};

    // 移除可能存在的旧配对
    snprintf(cmd, sizeof(cmd), "bluetoothctl remove %s", mac);
    execute_command(cmd, NULL, 0);
    sleep(1);

    // 配对
    snprintf(cmd, sizeof(cmd), "bluetoothctl pair %s", mac);
    execute_command(cmd, NULL, 0);
    sleep(2);

    update_status("connecting...");
    snprintf(cmd, sizeof(cmd), "bluetoothctl connect %s", mac);
    execute_command(cmd, NULL, 0);

    // 等待连接成功，最多尝试10秒
    int connected = 0;
    for(int i = 0; i < 10; i++) {
        sleep(1);
        // 检查连接状态
        snprintf(cmd, sizeof(cmd), "bluetoothctl info %s", mac);
        memset(output, 0, sizeof(output));
        execute_command(cmd, output, sizeof(output));
        if(strstr(output, "Connected: yes") != NULL) {
            connected = 1;
            break;
        }
        printf("[BT] Waiting for connection... %d/10\n", i + 1);
    }

    if(!connected) {
        update_status("connect failed");
        bp.connected_mac[0] = '\0';
        return;
    }

    // 确保设备已信任（有时需要trust）
    snprintf(cmd, sizeof(cmd), "bluetoothctl trust %s", mac);
    execute_command(cmd, NULL, 0);

    // 启动bluealsa（如果尚未运行）
    system("killall bluealsa 2>/dev/null");
    system("bluealsa -p a2dp-sink &");
    sleep(2); // 给bluealsa一些时间初始化

    // 验证bluealsa是否正常运行
    FILE * fp = popen("ps | grep bluealsa | grep -v grep", "r");
    if(fp) {
        char buf[128];
        if(fgets(buf, sizeof(buf), fp)) {
            printf("[BT] bluealsa is running\n");
            // 保存当前连接的MAC地址
            strcpy(bp.connected_mac, mac);
            update_status("connected");
        } else {
            update_status("bluealsa failed");
            bp.connected_mac[0] = '\0';
        }
        pclose(fp);
    } else {
        update_status("check failed");
        bp.connected_mac[0] = '\0';
    }
}

// 测试按钮点击事件
static void test_click(lv_event_t * e)
{
    lv_obj_t * btn   = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);

    if(!bp.is_playing) {
        // 检查是否有已连接的设备
        if(strlen(bp.connected_mac) == 0) {
            update_status("no device");
            return;
        }

        // 构造aplay命令
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "aplay -D \"bluealsa:DEV=%s,PROFILE=a2dp\" /mnt/app/factory/play_test.wav &",
                 bp.connected_mac);
        printf("[TEST] Executing: %s\n", cmd);
        system(cmd);

        // 更新按钮文字和状态
        lv_label_set_text(label, "stop");
        bp.is_playing = 1;
        update_status("playing test");
    } else {
        // 停止播放
        system("killall aplay 2>/dev/null");
        lv_label_set_text(label, "test");
        bp.is_playing = 0;
        update_status("stopped");
    }
}

// 执行命令并捕获输出
static void execute_command(const char * cmd, char * output, int max_len)
{
    printf("[CMD] %s\n", cmd);
    FILE * fp = popen(cmd, "r");
    if(!fp) {
        perror("popen");
        return;
    }

    if(output && max_len > 0) {
        size_t total = 0;
        while(total < max_len - 1) {
            size_t n = fread(output + total, 1, max_len - 1 - total, fp);
            if(n <= 0) break;
            total += n;
        }
        output[total] = '\0';
    } else {
        char buf[256];
        while(fgets(buf, sizeof(buf), fp)) {
            // 可打印调试信息
        }
    }

    int status = pclose(fp);
    printf("[CMD] exit status: %d\n", status);
}

// 更新状态标签
static void update_status(const char * status)
{
    lv_label_set_text(bp.status_label, status);
}