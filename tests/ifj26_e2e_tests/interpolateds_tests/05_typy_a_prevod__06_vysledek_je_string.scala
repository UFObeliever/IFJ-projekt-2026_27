import ifj26.*

@main
def main(): Unit =
  val n = 5
  val m: String = s"abc$n"
  val l = strLength(m)
  print(l)
