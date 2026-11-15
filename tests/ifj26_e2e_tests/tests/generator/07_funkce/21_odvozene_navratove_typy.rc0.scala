import ifj26.*

def sq(a: Int) = a * a
def name() = "ifj"
def nothing() =
  val u = 1

@main def main(): Unit =
  val r = sq(7)
  val n = name()
  nothing()
  print(r)
  print("\n")
  print(n)
  print("\n")
