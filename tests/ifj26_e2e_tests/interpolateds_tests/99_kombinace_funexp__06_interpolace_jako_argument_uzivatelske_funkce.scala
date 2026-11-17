import ifj26.*

def len(s: String): Int = strLength(s)

@main
def main(): Unit =
  val n = 5
  print(toStr(len(s"abc$n")))
