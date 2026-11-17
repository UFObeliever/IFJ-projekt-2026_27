import ifj26.*

def check(x: Int, strict: Boolean): Boolean =
  if strict then
    return x > 10
  else
    return x > 0

@main
def main(): Unit =
  val a = check(5, true)
  val b = check(5, false)
  print(a)
  print(b)
