import ifj26.*

@main def main(): Unit =
  val x = twice(4)
  print(x)

def twice(a: Int): Int =
  val b = once(a)
  return b + b

def once(a: Int): Int = a
