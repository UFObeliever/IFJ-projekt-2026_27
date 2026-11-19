import ifj26.*

def s2(x: String): String = x + x

def dbl(x: Int): Int = x * 2

def cat(a: String, b: String): String = a + b

@main
def main(): Unit =
  print(cat(s2("x") + "-", toStr(dbl(2))))
