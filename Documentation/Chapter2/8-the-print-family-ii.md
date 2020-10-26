# The print family ii

这一节我们来看 `printk` 中用于打印字符的四个辅助函数，以及由它们衍生出来的其它辅助函数的实现。

## _new_line

首先，是打印换行符的 `_new_line`。严格来说，换行符并不是打印出来的。我们只要把 frame buffer 的打印位置设置到下一行行首就好了：

```c
// `\n` handler
inline void _new_line() {
  pos.char_x = 0;
  pos.char_y += 1;

  _adjust_position();
}
```

因此，在 `_new_line` 的实现里，我们把标记当前位置的 `pos.char_x` 设置为 0，`pox_char_y` 加 1。但这里要考虑一个问题，就是当 `char_y` 超过了屏幕最大高度的情况。这时，我们有两个选择：

* 一个是让整个屏幕向上移动一行，让接下来所有的显示都只在最后一行上显示；
* 另一个是直接回到屏幕第一行显示；

前者更接近我们现在使用的终端，但实现起来比较麻烦。因此，暂时我们选择第二种方案。当然，除了高度之外，其实在水平方向显示的时候也会面临超过屏幕最大宽度的情况。为了统一处理这些屏幕坐标的问题，我们单独定义了一个辅助函数 `_adjust_position`，大家现在知道它的作用就好，最后我们统一来看它的实现。

## _backspace

第二个要介绍的函数是 `_backspace`。其实，所谓的退格，也不是打印出来的。我们只要把光标移动到上一个位置，然后在这个位置上显示一个空格，就模拟出删除上一个元素的效果了：

```c
// `\b` handler
inline void _backspace(int fg_color, int bg_color) {
  pos.char_x -= 1;
  _adjust_position();
  putchar(
    pos.fb_addr, pos.scn_width,
    pos.char_x * pos.char_width, pos.char_y * pos.char_height,
    fg_color, bg_color, ' ');
}
```

不过，就像刚才说的，移动光标之后，就可能存在从当前行的行首，移动到上一行行尾的情况。这时，我们还要调整 `char_y` 的值。而这个工作，也是委托给 `_adjust_position()` 函数完成的。

## _tab

第三个要介绍的，是打印制表符的 `_tab`。它的重点，在于计算移动到下一个制表符应该填充的空格。我们假定一个制表符是四个空格。因此：

* 如果当前光标位置是 4 的倍数，就应该显示 4 个空格移动到下一个位置；
* 否则，就计算当前位置和下一个 4 个倍数之间的值，显示和这个值相等的空格；

而这，就是下面给 `spaces` 变量赋值的代码的逻辑。计算出 `spaces` 后，每显示一个空格，我们就把光标水平位置加 1，并调整坐标就好了。

```c
inline void _tab(int fg_color, int bg_color) {
  int spaces = ((pos.char_x + 4 - 1) & ~(4 - 1)) - pos.char_x;

  while (spaces > 0) {
    putchar(
      pos.fb_addr, pos.scn_width, 
      pos.char_x * pos.char_width, pos.char_y * pos.char_height, 
      fg_color, bg_color, ' ');
    pos.char_x += 1;
    spaces -= 1;
    _adjust_position();
  }
}
```

## _ascii

第四个，是显示一般 ASCII 字符的 `_ascii`，它最简单，用 `putchar` 显示完之后，移动光标水平位置，并调整坐标就好了：

```c
inline void _ascii(unsigned char c, int fg_color, int bg_color) {
  putchar(
    pos.fb_addr, pos.scn_width, 
    pos.char_x * pos.char_width, pos.char_y * pos.char_height, 
    fg_color, bg_color, c);
  pos.char_x += 1;
  _adjust_position();
}
```

## _adjust_position

最后，就是这个我们一直提到的调整坐标位置的 `_adjust_position` 了。在四种情况下，它会调整坐标：

* 当光标已经在行首的时候，左移光标移动到上一行的行尾；
* 当光标已经在行尾的时候，右移光标移动到下一行的行首；
* 当光标已经在屏幕顶端的时候，无法继续向上一行移动光标；
* 当光标已经在屏幕底端的时候，下移光标将回到屏幕第一行；

```c
inline void _adjust_position() {
  if (pos.char_x < 0) {
    pos.char_x = pos.scn_width / pos.char_width - 1;
    pos.char_y -= 1;
  }

  if (pos.char_x >= (pos.scn_width / pos.char_width)) {
    pos.char_y += 1;
    pos.char_x = 0;
  }

  if (pos.char_y < 0) {
    pos.char_y = 0;
  }

  if (pos.char_y >= (pos.scn_height / pos.char_height)) {
    pos.char_y = 0;
  }
}
```

## What's next

以上，就是 `printk` 在显示字符时用到的几个辅助函数。现在，就剩下 `vsprintf` 没说了。实际上，这是整个打印 API 里，最复杂的一个。从下一节开始，我们就来实现它。
