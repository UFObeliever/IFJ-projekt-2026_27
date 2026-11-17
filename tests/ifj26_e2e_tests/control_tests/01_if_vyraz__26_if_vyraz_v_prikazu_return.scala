import ifj26.*

def absv(n: Int): Int =
  return if n > 0 then n else 0 - n

@main
def main(): Unit =
  val r = absv(-4)
  print(r)
