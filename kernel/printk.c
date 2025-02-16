#include "printk.h"
#include "lib.h"

/**
 * 初始化VBE信息
 */
void init_vbe_info(void) {
    printk_pos.x_resolution = 1024;
    printk_pos.y_resolution = 768;

    printk_pos.x_position = 0;
    printk_pos.y_position = 0;

    printk_pos.x_char_size = 8;
    printk_pos.y_char_size = 16;

    printk_pos.fb_addr = (uint32_t *)0xffff800001a00000;
    printk_pos.fb_length = (printk_pos.x_resolution * printk_pos.y_resolution * 4);
    printk_pos.bpp = 32;
}

/**
 * @brief 在当前打印位置绘制一个颜色字符。
 *
 * 该函数从全局数组 font_ascii 中读取指定的字符（由参数 font 决定）字形数据，  
 * 然后将它以 8x16 像素的尺寸写入到由全局变量 printk_pos 指定的帧缓冲区。  
 * 在绘制过程中，函数会根据字形数据的像素位来决定是使用前景色（char_color）  
 * 还是背景色（bg_color）。
 *
 * @param char_color 字符前景色。
 * @param bg_color   字符背景色。
 * @param font       字体（或 ASCII 字符）在 font_ascii 数组中的索引。
 *
 * @note 
 * 1. 函数依赖全局的 printk_pos 结构来获取绘制位置和帧缓冲区地址，  
 *    请在调用前确保 printk_pos 的各字段已正确设置。  
 * 2. 每次调用只负责绘制一个 8x16 大小的字符。若需连续打印多个字符，  
 *    需在外部循环中多次调用本函数，并更新 printk_pos。  
 */
void put_color_char(uint32_t char_color, uint32_t bg_color, uint8_t font) {
    uint8_t *fontp = font_ascii[font];
    uint8_t *byte_fb = (uint8_t *)printk_pos.fb_addr;

    for (int32_t i = 0; i < 16; i++) {    // 每行16像素
        for (int32_t j = 0; j < 8; j++) { // 每字符8像素宽
            // 计算当前像素的字节偏移（修正坐标计算）
            int32_t y_offset = (printk_pos.y_position + i) * printk_pos.x_resolution * 4;
            int32_t x_offset = (printk_pos.x_position + j) * 4;
            uint8_t *pixel = byte_fb + y_offset + x_offset;

            // 设置颜色
            if (fontp[i] & (0x80 >> j)) {
                *(uint32_t *)pixel = char_color;
            } else {
                *(uint32_t *)pixel = bg_color;
            }
        }
    }
}

/**
 * 数值格式化函数 - 将整数转换为字符串
 * @str:   目标缓冲区当前位置
 * @end:   缓冲区末尾（不可越界）
 * @num:   待格式化的数值
 * @base:  进制（2~36）
 * @width: 字段总宽度
 * @prec:  精度（最小数字位数）
 * @flags: 格式化标志（见宏定义）
 * 返回值：更新后的缓冲区位置
 */
static int8_t *number(int8_t *str, const int8_t *end, int64_t num, int32_t base, int32_t width, int32_t prec,
                      int32_t flags) {
    int8_t sign = 0;      // 符号字符（+/-/空格）
    int8_t tmp[72];       // 临时存储逆序数字（64位二进制最大长度+冗余）
    const int8_t *digits; // 数字字符集
    int32_t i = 0;        // tmp数组索引
    int32_t pad_len;      // 填充字符数量
    int8_t pad_char;      // 填充字符（'0'或' '）

    /* 1. 初始化数字字符集（大小写） */
    digits = (flags & SMALL) ? "0123456789abcdefghijklmnopqrstuvwxyz" : "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    /* 2. 校验进制合法性 */
    if (base < 2 || base > 36) {
        return str; // 非法进制直接返回
    }

    /* 3. 处理左对齐时禁用零填充 */
    if (flags & LEFT) {
        flags &= ~ZEROPAD;
    }

    /* 4. 确定填充字符 */
    pad_char = (flags & ZEROPAD) ? '0' : ' ';

    /* 5. 处理符号 */
    if (flags & SIGN) {
        if (num < 0) {
            sign = '-';
            num = -num; // 转为正数处理
        } else if (flags & PLUS) {
            sign = '+';
        } else if (flags & SPACE) {
            sign = ' ';
        }
    }

    /* 6. 处理特殊前缀（0、0x）对宽度的占用 */
    if (flags & SPECIAL) {
        if (base == 8) {
            width--; // 八进制前缀 "0" 占1位
        } else if (base == 16) {
            width -= 2; // 十六进制前缀 "0x" 占2位
        }
    }

    /* 7. 将数值转换为逆序字符数组 */
    if (num == 0) {
        tmp[i++] = '0'; // 直接处理0的情况
    } else {
        while (num != 0 && i < sizeof(tmp)) {
            int64_t rem = num % base;
            num /= base;
            tmp[i++] = digits[rem]; // 取余得到当前位字符
        }
    }

    /* 8. 计算实际精度（至少输出prec位数字） */
    if (i > prec) {
        prec = i; // 若数字位数超过精度，按实际位数处理
    }
    pad_len = prec - i; // 需要补零的数量

    /* 9. 计算剩余字段宽度（总宽度 - 符号 - 特殊前缀 - 数字位数 - 补零） */
    width -= (sign ? 1 : 0) + (prec > i ? prec : i) + (flags & SPECIAL ? (base == 8 ? 1 : 2) : 0);

    /* 10. 处理非左对齐且非零填充时的前导空格 */
    if (!(flags & (ZEROPAD | LEFT)) && width > 0) {
        while (width-- > 0 && str < end) {
            *str++ = ' ';
        }
    }

    /* 11. 写入符号 */
    if (sign && str < end) {
        *str++ = sign;
    }

    /* 12. 写入特殊前缀（0x或0） */
    if (flags & SPECIAL) {
        if (base == 8 && str < end) { // 八进制前缀
            *str++ = '0';
        } else if (base == 16 && str < end - 1) { // 十六进制前缀
            *str++ = '0';
            *str++ = (flags & SMALL) ? 'x' : 'X';
        }
    }

    /* 13. 零填充或数字前的填充 */
    if (!(flags & LEFT) && (flags & ZEROPAD) && width > 0) {
        while (width-- > 0 && str < end) {
            *str++ = pad_char;
        }
    }

    /* 14. 写入补零（满足精度要求） */
    while (pad_len-- > 0 && str < end) {
        *str++ = '0';
    }

    /* 15. 逆序写入实际数字 */
    while (i-- > 0 && str < end) {
        *str++ = tmp[i]; // tmp中存储的是逆序字符，需反向输出
    }

    /* 16. 左对齐时的尾部填充 */
    if (flags & LEFT && width > 0) {
        while (width-- > 0 && str < end) {
            *str++ = ' ';
        }
    }

    return str;
}

/**
 * 安全版本的vsnprintf - 将格式化字符串写入缓冲区
 * @buf: 目标缓冲区
 * @size: 缓冲区最大容量（包含结尾的'\0'）
 * @fmt: 格式化字符串
 * @args: 可变参数列表
 * 返回值：写入的字符数（不含结尾'\0'），若缓冲区不足则返回理论需要的字符数
 */
int32_t vsnprintf(int8_t *buf, size_t size, const char *fmt, va_list args) {
    int8_t *str = buf;                    // 当前写入位置
    const int8_t *const end = buf + size; // 允许写入的最后一个位置（保留结尾'\0'）

    /* 若缓冲区大小为0，直接返回计算所需长度 */
    if (size == 0)
        return 0;

    /* 遍历格式字符串 */
    for (; *fmt && str < end; fmt++) {
        /* 非格式字符直接写入 */
        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }

        /* 解析格式标志 */
        uint32_t flags = 0;
        const char *start = fmt; // 记录格式起始位置，用于错误回退

        /* 解析标志字符：- + # 0 等 */
        while (1) {
            fmt++;
            switch (*fmt) {
            case '-':
                flags |= LEFT;
                break;
            case '+':
                flags |= PLUS;
                break;
            case ' ':
                flags |= SPACE;
                break;
            case '#':
                flags |= SPECIAL;
                break;
            case '0':
                flags |= ZEROPAD;
                break;
            default:
                goto parse_width; // 结束标志解析
            }
        }

    parse_width:
        /* 解析字段宽度（支持*和数字） */
        int32_t field_width = -1;
        if (*fmt == '*') {
            fmt++;
            field_width = va_arg(args, int32_t);
            if (field_width < 0) { // 负宽度视为左对齐正宽度
                field_width = -field_width;
                flags |= LEFT;
            }
        } else if (is_digit(*fmt)) {
            field_width = 0;
            while (is_digit(*fmt)) {
                field_width = field_width * 10 + (*fmt - '0');
                fmt++;
            }
        }

    parse_precision:
        /* 解析精度（.后接数字或*） */
        int32_t precision = -1;
        if (*fmt == '.') {
            fmt++;
            if (*fmt == '*') {
                fmt++;
                precision = va_arg(args, int32_t);
            } else if (is_digit(*fmt)) {
                precision = 0;
                while (is_digit(*fmt)) {
                    precision = precision * 10 + (*fmt - '0');
                    fmt++;
                }
            }
            if (precision < 0)
                precision = 0; // 精度至少为0
        }

        /* 解析长度修饰符：h, l, ll */
        uint8_t qualifier = 0;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
            qualifier = *fmt;
            fmt++;
            if (qualifier == 'l' && *fmt == 'l') { // 处理ll
                qualifier = '2';
                fmt++;
            }
        }

        /* 处理格式字符 */
        switch (*fmt) {
        case 'c': { // 字符
            /* 计算填充空格数 */
            int32_t padding = (field_width > 1) ? (field_width - 1) : 0;
            char c = (char)va_arg(args, int32_t);

            /* 左对齐：先填充后内容 */
            if (!(flags & LEFT)) {
                while (padding-- > 0 && str < end)
                    *str++ = ' ';
            }

            if (str < end)
                *str++ = c;

            /* 右对齐：内容后填充 */
            if (flags & LEFT) {
                while (padding-- > 0 && str < end)
                    *str++ = ' ';
            }
            break;
        }

        case 's': { // 字符串
            char *s = va_arg(args, char *);
            if (!s)
                s = "(null)";

            /* 计算实际拷贝长度（考虑精度） */
            size_t len = strlen(s);
            if (precision >= 0 && len > (size_t)precision) {
                len = precision;
            }

            /* 计算填充空格数 */
            int32_t padding = (field_width > len) ? (field_width - len) : 0;

            /* 左对齐：先填充后内容 */
            if (!(flags & LEFT)) {
                while (padding-- > 0 && str < end)
                    *str++ = ' ';
            }

            /* 拷贝字符串内容 */
            size_t copy_len = (str + len <= end) ? len : end - str;
            if (copy_len > 0) {
                memcpy(str, s, copy_len);
                str += copy_len;
            }

            /* 右对齐：内容后填充 */
            if (flags & LEFT) {
                while (padding-- > 0 && str < end)
                    *str++ = ' ';
            }
            break;
        }

        case 'p': {                           // 指针
            flags |= SPECIAL | SMALL;         // 强制0x前缀和小写
            field_width = sizeof(void *) * 2; // 默认宽度为指针长度两倍
            flags |= ZEROPAD;
            uint64_t num = (uint64_t)va_arg(args, void *);
            str = number(str, end, num, 16, field_width, precision, flags);
            break;
        }

        /* 数值类型（d, i, u, o, x, X） */
        case 'd':
        case 'i':
            flags |= SIGN; // 有符号数
                           // 无break，继续处理无符号逻辑

        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            uint64_t num = 0;
            int base = 10;

            /* 设置进制 */
            switch (*fmt) {
            case 'o':
                base = 8;
                break;
            case 'x':
            case 'X':
                base = 16;
                break;
            }

            /* 处理大小写 */
            if (*fmt == 'x')
                flags |= SMALL;

            /* 解析数值（考虑长度修饰符） */
            if (flags & SIGN) { // 有符号数
                int64_t signed_num;
                switch (qualifier) {
                case 'l':
                    signed_num = va_arg(args, int32_t);
                    break;
                case '2':
                    signed_num = va_arg(args, int64_t);
                    break; // ll
                case 'h':
                    signed_num = (int16_t)va_arg(args, int32_t);
                    break;
                default:
                    signed_num = va_arg(args, int32_t);
                }
                num = (signed_num < 0) ? -signed_num : signed_num;
            } else { // 无符号数
                switch (qualifier) {
                case 'l':
                    num = va_arg(args, uint32_t);
                    break;
                case '2':
                    num = va_arg(args, uint64_t);
                    break; // ll
                case 'h':
                    num = (uint16_t)va_arg(args, uint32_t);
                    break;
                default:
                    num = va_arg(args, uint32_t);
                }
            }

            /* 调用数值格式化函数 */
            str = number(str, end, num, base, field_width, precision, flags);
            break;
        }

        case 'n': { // 写入已处理字符数
            void *ptr = va_arg(args, void *);
            if (qualifier == '2') { // ll
                *(int64_t *)ptr = str - buf;
            } else if (qualifier == 'l') {
                *(int32_t *)ptr = str - buf;
            } else {
                *(int16_t *)ptr = str - buf;
            }
            break;
        }

        case '%': // 转义%
            if (str < end)
                *str++ = '%';
            break;

        default: // 无效格式符
            /* 回退到原始字符 */
            fmt = start; // 重置到%位置
            if (str < end)
                *str++ = *fmt;
            break;
        }
    }

    /* 确保缓冲区以'\0'结尾 */
    *str = '\0';

    /* 返回实际写入长度（不含结尾符） */
    return (str - buf < size) ? (str - buf) : (int32_t)size - 1;
}

/**
 * @brief 使用指定前景色和背景色格式化打印字符串，并处理控制字符
 * 
 * 该函数将格式化后的字符串输出到屏幕，支持换行符(\\n)、退格符(\\b)、制表符(\\t)等特殊字符，
 * 自动处理屏幕边界回绕和位置计算。使用全局显示位置状态(printk_pos)跟踪当前输出位置。
 * 
 * @param char_color 字符颜色代码 (RGB或预设颜色值，取决于系统实现)
 * @param bg_color 背景颜色代码 (格式同char_color)
 * @param fmt 格式化字符串，语法与printf一致
 * @param ... 可变参数列表，匹配格式化字符串中的占位符
 * 
 * @return int32_t 实际生成的字符数量(可能因缓冲区限制被截断)
 * 
 * @note 特殊字符处理规则：
 * - 换行符(\n)：将光标移动到下一行行首
 * - 退格符(\b)：光标回退一个字符位置，若越界则移动到上一行末尾
 * - 制表符(\t)：按8字符宽度对齐，用空格填充到下一个对齐位置
 * - 屏幕边界：当超出屏幕分辨率时自动回绕到对应边界
 * 
 * @warning 
 * - 使用内部固定大小缓冲区(printk_buf)，最大输出长度为sizeof(printk_buf)-1
 * - 修改全局显示状态printk_pos，非线程安全
 * - 颜色参数有效性取决于底层put_color_char实现
 * 
 * @example
 * color_printk(RED, BLACK, "Error: %d\\t%s\\n", err_code, message);
 */
int32_t vcolor_printk(uint32_t char_color, uint32_t bg_color, const char *fmt, va_list args) {
    int32_t i = 0;
    int32_t count = 0;
    int32_t line = 0;
    
    i = vsnprintf(printk_buf, sizeof(printk_buf), fmt, args);

    // 处理缓冲区溢出
    if (i >= sizeof(printk_buf)) {
        i = sizeof(printk_buf) - 1;
        printk_buf[i] = '\0'; // 强制终止
    }

    for (count = 0; count < i || line; count++) {
        if (line > 0) {
            count--;
            goto Label_tab;
        }
        if ((unsigned char)*(printk_buf + count) == '\n') {
            printk_pos.y_position += printk_pos.y_char_size;
            printk_pos.x_position = 0;
        } else if ((unsigned char)*(printk_buf + count) == '\b') {
            printk_pos.x_position -= printk_pos.x_char_size;
            if (printk_pos.x_position < 0) {
                printk_pos.x_position = printk_pos.x_resolution - printk_pos.x_char_size;
                printk_pos.y_position -= printk_pos.y_char_size;
                if (printk_pos.y_position < 0) {
                    printk_pos.y_position = printk_pos.y_resolution - printk_pos.y_char_size;
                }
            }
            put_color_char(char_color, bg_color, ' ');
        } else if ((unsigned char)*(printk_buf + count) == '\t') {
            int current_char_x = printk_pos.x_position / printk_pos.x_char_size;
            line = ((current_char_x + 8) & ~(8 - 1)) - current_char_x;

        Label_tab:
            line--;
            put_color_char(char_color, bg_color, ' ');
            printk_pos.x_position += printk_pos.x_char_size;
        } else {
            put_color_char(char_color, bg_color, (unsigned char)*(printk_buf + count));
            printk_pos.x_position += printk_pos.x_char_size;
        }

        // 边界检查
        if (printk_pos.x_position >= printk_pos.x_resolution) {
            printk_pos.y_position += printk_pos.y_char_size;
            printk_pos.x_position = 0;
        }
        if (printk_pos.y_position >= printk_pos.y_resolution) {
            // 这里是直接返回第一行开始打印，按照现在的日志输出直觉，可能进行页面滚动会比较好
            printk_pos.y_position = 0;
        }
    }
    return i;
}

/**
 * @brief 带颜色格式化输出函数
 * @param char_color 字符颜色代码
 * @param bg_color 背景颜色代码
 * @param fmt 格式化字符串
 * @return 实际输出的字符数
 */
int32_t color_printk(uint32_t char_color, uint32_t bg_color, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int32_t ret = vcolor_printk(char_color, bg_color, fmt, args);
    va_end(args);
    return ret;
}

/**
 * @brief 内核默认打印函数
 * @brief 使用白色前景/黑色背景格式化输出
 * @param fmt 格式化字符串
 * @return 实际输出的字符数
 */
int32_t printk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int32_t ret = vcolor_printk(WHITE, BLACK, fmt, args);
    va_end(args);
    return ret;
}


/**
 * @brief 通用日志实现核心
 * @param fg 前景色
 * @param bg 背景色
 * @param tag 日志标签（需包含空格等格式）
 * @param fmt 原始格式字符串
 * @param args 可变参数列表
 * @return 实际输出字符数（包含标签）
 */
static int32_t logk_impl(uint32_t fg, uint32_t bg, const char* tag, const char* fmt, va_list args) {
    // 后续对该操作添加原子锁
    int32_t tag_len = strlen(tag);
    int32_t tag_ret = color_printk(fg, bg, tag);
    int32_t body_ret = vcolor_printk(fg, bg, fmt, args);
    return (tag_ret < 0 || body_ret < 0) ? -1 : tag_ret + body_ret;
}

/**
 * @brief 通用日志实现
 */
int32_t logk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int32_t ret = logk_impl(WHITE, BLACK, "[INFO] ", fmt, args);
    va_end(args);
    return ret;
}

/**
 * @brief 通用警告实现
 */
int32_t warnk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int32_t ret = logk_impl(YELLOW, BLACK, "[WARN] ", fmt, args);
    va_end(args);
    return ret;
}

/**
 * @brief 通用错误实现
 */
int32_t errk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int32_t ret = logk_impl(RED, BLACK, "[ERR]  ", fmt, args);
    va_end(args);
    return ret;
}

/**
 * @brief 通用致命错误实现
 */
int32_t fatalk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int32_t ret = logk_impl(RED, WHITE, "[FATL] ", fmt, args);
    va_end(args);
    return ret;
}
