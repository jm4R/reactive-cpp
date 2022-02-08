# reactive-cpp

Library is in development, but here's an already-working demo:

```cpp
using namespace circle;

property<int> a = 0;
property<int> b = 0;

property max = BIND(a, b, std::max(a, b));

a = 60;
b = 75;
std::cout << "max(" << *a  << ", " << *b << ") = " << *max << std::endl;

a = 120;
std::cout << "max(" << *a  << ", " << *b << ") = " << *max << std::endl;
```

The example prints:

> max(60, 75) = 75\
> max(120, 75) = 120