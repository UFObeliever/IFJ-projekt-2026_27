import ifj26.*

def noisy(a: Int)(b: Int): Int =
  print("n")
  a + b

@main
def main(): Unit =
  noisy(1)(2)
  noisy(3)(4)
