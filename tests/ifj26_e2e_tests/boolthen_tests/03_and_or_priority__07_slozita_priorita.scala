import ifj26.*

@main
def main(): Unit =
  val a = true
  val b = false
  val c = false
  val d = false
  val r = a || b && c || d
  print(r)
