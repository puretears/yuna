# The print family iv

这一节，我们实现 `vsprintf` 的 `fmt` 参数的处理，主要就是识别 `fmt` 中各种指定格式的标识符。

## vsprintf 的格式规范

在实现之前，先来看下 C 中 `vsprintf` 的格式规范：

```c
// %[flags][width][.precision][length]specifier
```
也就是说，每一个格式标识符，写完整的话，最多包含五部分。其中：

* `flags` 部分用于指定是否在数字前显示符号；
* `width / precision` 分别表示格式化后字符串的宽度以及数字的宽度，这和上一节我们实现 `_number` 时的两个同名参数的含义是相同的；
* `length` 是一个和数据长度有关的选项，例如，按照长整形显示整数；
* `specifier` 是数据格式的标识符，用于指定数据的类型，数字的数制等信息；

## 实现格式规范的辅助函数

为了处理标识符中的每一部分，在 printk.c 里，我们定义了 5 个辅助函数，我们一个个来看下。

### _parse_flags

首先，是 `_parse_flags`：

```c
int _parse_flags(const char **fmt) {
  int flags = 0;
  
  while (1) {
    if (**fmt == '-') { flags |= LEFT; }
    else if (**fmt == '+') { flags |= PLUS; }
    else if (**fmt == ' ') { flags |= SPACE; }
    else if (**fmt == '#') { flags |= SPECIAL; }
    else {
      break;
    }

    ++(*fmt);
  }

  return flags;
}
```

这里，`fmt` 是传递给 `vsprintf` 的格式描述符，由于在 `_parse_flags` 里我们会随着解析字符串移动 `fmt` 的当前位置，因此，这里传递了 `const char **`。在它的实现里，我们支持了 4 种 C 函数支持的标志 `- / + / / #`，在遍历 `fmt` 的过程中，每读到这些字符，就把 `flags` 中对应的 bit 置位，最后返回 `flags` 就好了。

### _parse_width

其次，是 `_parse_width`：

```c
int _parse_width(const char **fmt) {
  int width = -1;

  if (is_digit(**fmt)) {
    width = _atoi(fmt);
  }

  // TODO: we omit the `*` case for simplicity.
  return width;
}
```

我们要实现的逻辑，就是把 `fmt` 指向的表达宽度的字符串，转换成数字并返回。这里，使用了另外一个辅助函数 `_atoi`，它的定义是这样的：

```c
#define is_digit(c) ((c) >= '0' && (c) <= '9')

int _atoi(const char **s) {
  int i = 0;

  while (is_digit(**s)) {
    i = i * 10 + **s - '0';
    ++(*s);
  }

  return i;
}
```

思路就是根据当前字符到 `'0'` 的距离得到对应的数字，每读一个新的数位，就把之前的数字乘以 10，这样扫描完 `s` 之后，就是 `s` 指向的字符串对应的 10 进制数了。于是，在 `_parse_width` 的实现里，只要 `fmt` 当前指向的内容是数字，我们就把它传递给 `_atoi` 解析，并返回解析后的结果就好了。

### _parse_precision

接下来，是解析数字位数的 `_parse_precision`，本质上说，实现的方法和 `_parse_width` 是相同的，都是把字符串中的数字解析成整数，因此我们就不再重复了：

```c
int _parse_precision(const char **fmt) {
  int precision = -1;

  if (**fmt == '.') {
    ++(*fmt);

    if (is_digit(**fmt)) { precision = _atoi(fmt); }
    // TODO: we omit the `*` case for simplicity
  }

  return precision;
}
```

值得特别说明一下的就是，就像注释中 `// TODO` 提到的，我们没有实现这两个选项中 `*` 的功能。`*` 表示通过额外的参数而不是格式描述符中的值来指定相应的宽度和精度。在内核开发里，暂时我们不会用到，简单起见，就先不实现了，毕竟，当前的代码已经足够麻烦了 :)

### _parse_sub_specifier

这一节最后，是解析数据长度的标识符。当前，我们只支持一个选项，就是 `'l'`，表示按照长整形显示数字：

```c
int _parse_sub_specifier(const char **fmt) {
  int specifier = -1;
  // TODO: we omit other specifiers for simplicity
  if (**fmt == 'l') {
    specifier = **fmt;
    ++(*fmt);
  }

  return specifier;
}
```

## What's next

以上，就是 `vsprintf` 的实现要用到的最后四个辅助函数。现在，我们已经有了数制转换的 `_number`，以及解析格式标识符的这四个方法。下一节，我们就来实现 `vsprintf` 的完整流程。
