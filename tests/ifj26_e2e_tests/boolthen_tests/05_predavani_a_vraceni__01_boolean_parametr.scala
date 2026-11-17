import ifj26.*

def pick(b: Boolean): Int =
  if b then
    return 1
  else
    return 0

@main
def main(): Unit =
  val a = pick(true)
  val b = pick(false)
  print(a)
  print(b)
