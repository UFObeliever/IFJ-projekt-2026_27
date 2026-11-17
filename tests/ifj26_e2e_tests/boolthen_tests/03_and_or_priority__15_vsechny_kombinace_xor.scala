import ifj26.*

@main
def main(): Unit =
  val t = true
  val f = false
  val r1 = (f || f) && !(f && f)
  val r2 = (f || t) && !(f && t)
  val r3 = (t || f) && !(t && f)
  val r4 = (t || t) && !(t && t)
  print(r1)
  print(r2)
  print(r3)
  print(r4)
