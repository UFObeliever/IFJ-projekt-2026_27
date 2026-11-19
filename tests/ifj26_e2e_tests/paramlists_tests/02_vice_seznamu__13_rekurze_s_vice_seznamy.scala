import ifj26.*

def sum(n: Int)(acc: Int): Int =
  if n == 0 then
    return acc
  else
    return sum(n - 1)(acc + n)

@main
def main(): Unit =
  val r = sum(10)(0)
  print(r)
