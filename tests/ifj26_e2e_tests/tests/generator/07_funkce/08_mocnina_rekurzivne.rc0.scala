import ifj26.*

def pow(b: Int, e: Int): Int =
  if e == 0 then
    return 1
  else
    val e1 = e - 1
    val r = pow(b, e1)
    return b * r

@main def main(): Unit =
  val p = pow(3, 13)
  print(p)
  print("\n")
