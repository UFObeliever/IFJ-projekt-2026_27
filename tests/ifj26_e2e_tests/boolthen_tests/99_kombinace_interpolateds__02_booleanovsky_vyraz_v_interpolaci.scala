import ifj26.*

@main
def main(): Unit =
  val a = true
  val b = false
  val m = s"${a && b}/${a || b}"
  print(m)
