import ifj26.*

def one(): Int = 1

def two(): Int = 2

@main
def main(): Unit =
  val a = one()
  val b = two()
  val r = if a > b then a else b
  print(r)
