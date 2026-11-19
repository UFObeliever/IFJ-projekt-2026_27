import ifj26.*

@main
def main(): Unit =
  val t = true
  val f = false
  val a1 = (!(t && f)) == (!t || !f)
  val a2 = (!(f || f)) == (!f && !f)
  val a3 = (!(t || f)) == (!t && !f)
  print(a1)
  print(a2)
  print(a3)
