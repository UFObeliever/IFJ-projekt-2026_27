import ifj26.*

def mid(a: Int)()(b: Int): Int = a * 10 + b

@main
def main(): Unit =
  val r = mid(1)()(2)
  print(r)
