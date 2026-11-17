import ifj26.*

def pick(c: Boolean) = if c then 1 else 2

@main
def main(): Unit =
  val a = pick(true)
  val b = pick(false)
  print(a)
  print(b)
