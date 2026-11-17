import ifj26.*

@main
def main(): Unit =
  val a = 12
  val m = s"""x
$a"""
  val n = strLength(m)
  print(n)
