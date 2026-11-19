import ifj26.*

def isEven(n: Int): Boolean = n / 2 * 2 == n

@main
def main(): Unit =
  val a = if isEven(4) then "even" else "odd"
  val b = if isEven(3) then "even" else "odd"
  print(a)
  print(",")
  print(b)
