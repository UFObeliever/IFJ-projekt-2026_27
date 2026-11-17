import ifj26.*

def twice(x: Int): Int = x * 2

@main
def main(): Unit =
  val n = 4
  print(toStr(twice(strLength(s"$n$n"))))
