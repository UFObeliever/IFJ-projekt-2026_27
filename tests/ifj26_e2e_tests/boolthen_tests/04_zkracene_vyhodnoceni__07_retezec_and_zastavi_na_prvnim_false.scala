import ifj26.*

@main
def main(): Unit =
  val z = 0
  val a = false
  val r = a && z == 0 && 10 / z > 1
  print(r)
