#include "page_txt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*********************
 * DEFINES
 *********************/
#define MAX_LINES 9             // 固定9行
#define LINE_MAX_WEIGHT 52      // 26个英文宽 (26*2)
#define WEIGHT_EN 2             // 英文权重
#define WEIGHT_CN 3             // 中文权重
#define PAGE_BUFFER_SIZE 2048   // 渲染缓冲区
#define MAX_PAGE_HISTORY 200    // 最大记录200页索引

/**********************
 * STATIC VARIABLES
 **********************/
static lv_obj_t * screen = NULL;
static lv_obj_t * text_label = NULL;
static lv_obj_t * page_label = NULL;

static FILE * current_fp = NULL;
static long file_total_size = 0;
static int current_page_idx = 0;
static long page_offsets[MAX_PAGE_HISTORY]; // 记录每一页起始位置
static char display_buffer[PAGE_BUFFER_SIZE];

/**********************
 * STATIC FUNCTIONS
 **********************/

// 仅判断是否为纯英文字母
static bool is_pure_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// 获取UTF-8字符长度
static int get_utf8_len(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static void update_display(void) {
    if (!current_fp) return;

    // 跳转到当前页起始位置
    fseek(current_fp, page_offsets[current_page_idx], SEEK_SET);
    
    memset(display_buffer, 0, sizeof(display_buffer));
    int buf_idx = 0;
    int line_cnt = 0;
    int cur_line_weight = 0;

    while (line_cnt < MAX_LINES) {
        int c = fgetc(current_fp);
        if (c == EOF) break;

        unsigned char uc = (unsigned char)c;

        // 1. 处理换行逻辑 (n-1 逻辑)
        if (uc == '\n' || uc == '\r') {
            if (uc == '\r') { // 处理CRLF
                int next_c = fgetc(current_fp);
                if (next_c != '\n' && next_c != EOF) ungetc(next_c, current_fp);
            }
            
            int peek_c = fgetc(current_fp);
            if (peek_c == '\n' || peek_c == '\r') {
                // 发现连续换行: 计入一行，并存入换行符
                display_buffer[buf_idx++] = '\n';
                line_cnt++;
                cur_line_weight = 0;
                ungetc(peek_c, current_fp); 
            } else {
                // 单个换行: 视为空格，不增加行数
                if (cur_line_weight + WEIGHT_EN <= LINE_MAX_WEIGHT) {
                    display_buffer[buf_idx++] = ' ';
                    cur_line_weight += WEIGHT_EN;
                }
                if (peek_c != EOF) ungetc(peek_c, current_fp);
            }
            continue;
        }

        // 2. 纯英文单词保护 (Word Wrap)
        if (is_pure_alpha(uc)) {
            int word_weight = WEIGHT_EN;
            long saved_pos = ftell(current_fp);
            
            // 向后预读单词长度
            int tc;
            while ((tc = fgetc(current_fp)) != EOF && is_pure_alpha((char)tc)) {
                word_weight += WEIGHT_EN;
            }
            fseek(current_fp, saved_pos, SEEK_SET); // 退回起点

            // 如果该单词在当前行放不下，且该单词总长没超过一行上限
            if (cur_line_weight + word_weight > LINE_MAX_WEIGHT && word_weight <= LINE_MAX_WEIGHT) {
                display_buffer[buf_idx++] = '\n';
                line_cnt++;
                cur_line_weight = 0;
                if (line_cnt >= MAX_LINES) {
                    ungetc(uc, current_fp); // 退回触发单词的第一个字母
                    break;
                }
            }
        }

        // 3. 权重与折行计算
        int char_len = get_utf8_len(uc);
        int weight = (char_len > 1) ? WEIGHT_CN : WEIGHT_EN;

        if (cur_line_weight + weight > LINE_MAX_WEIGHT) {
            display_buffer[buf_idx++] = '\n';
            line_cnt++;
            cur_line_weight = 0;
            if (line_cnt >= MAX_LINES) {
                ungetc(uc, current_fp);
                break;
            }
        }

        // 4. 将完整字符存入缓冲区
        display_buffer[buf_idx++] = uc;
        for (int k = 1; k < char_len; k++) {
            int tc = fgetc(current_fp);
            if (tc != EOF) display_buffer[buf_idx++] = (unsigned char)tc;
        }
        cur_line_weight += weight;

        // 防止缓冲区溢出
        if (buf_idx >= PAGE_BUFFER_SIZE - 10) break;
    }

    display_buffer[buf_idx] = '\0';
    lv_label_set_text(text_label, display_buffer);

    // 记录下一页的起始位置
    if (current_page_idx + 1 < MAX_PAGE_HISTORY) {
        page_offsets[current_page_idx + 1] = ftell(current_fp);
    }

    // 更新页码百分比
    char info[32];
    int percent = (file_total_size > 0) ? (int)((ftell(current_fp) * 100) / file_total_size) : 0;
    snprintf(info, sizeof(info), "%d%%", (percent > 100) ? 100 : percent);
    lv_label_set_text(page_label, info);
}

static void next_page_click(lv_event_t * e) {
    // 检查是否有下一页
    if (page_offsets[current_page_idx + 1] > 0 && page_offsets[current_page_idx + 1] < file_total_size) {
        current_page_idx++;
        update_display();
    }
}

static void prev_page_click(lv_event_t * e) {
    if (current_page_idx > 0) {
        current_page_idx--;
        update_display();
    }
}

static void back_click(lv_event_t * e) {
    if (current_fp) {
        fclose(current_fp);
        current_fp = NULL;
    }
    // 这里调用您的页面跳转逻辑，例如:
    // lv_obj_del(screen);
}

lv_obj_t * page_txt(char * filename) {
    if (current_fp) fclose(current_fp);
    current_fp = fopen(filename, "rb");
    if (!current_fp) return NULL;

    fseek(current_fp, 0, SEEK_END);
    file_total_size = ftell(current_fp);
    fseek(current_fp, 0, SEEK_SET);

    memset(page_offsets, 0, sizeof(page_offsets));
    page_offsets[0] = 0;
    current_page_idx = 0;

    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);

    text_label = lv_label_create(screen);
    lv_obj_set_width(text_label, lv_pct(94));
    lv_obj_align(text_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_long_mode(text_label, LV_LABEL_LONG_CLIP); 

    page_label = lv_label_create(screen);
    lv_obj_align(page_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    // 翻页控制按钮
    lv_obj_t * btn_prev = lv_btn_create(screen);
    lv_obj_set_size(btn_prev, 60, 40);
    lv_obj_align(btn_prev, LV_ALIGN_BOTTOM_LEFT, 10, -5);
    lv_obj_add_event_cb(btn_prev, prev_page_click, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn_prev), "Prev");

    lv_obj_t * btn_next = lv_btn_create(screen);
    lv_obj_set_size(btn_next, 60, 40);
    lv_obj_align(btn_next, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
    lv_obj_add_event_cb(btn_next, next_page_click, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn_next), "Next");

    update_display();
    return screen;
}