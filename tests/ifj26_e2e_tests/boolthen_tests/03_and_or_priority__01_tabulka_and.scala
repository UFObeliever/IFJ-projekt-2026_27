import ifj26.*

@main
def main(): Unit =
  val t = true
  val f = false
  val r1 = t && t
  val r2 = t && f
  val r3 = f && t
  val r4 = f && f
  print(r1)
  print(r2)
  print(r3)
  print(r4)
