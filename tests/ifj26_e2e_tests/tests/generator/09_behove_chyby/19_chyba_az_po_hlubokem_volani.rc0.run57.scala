import ifj26.*

def down(n: Int): Int =
  if n == 0 then
    val z = 0
    val q = 1 / z
    return q
  else
    val m = n - 1
    val r = down(m)
    return r

@main def main(): Unit =
  print("go\n")
  val x = down(50)
  print("never\n")
