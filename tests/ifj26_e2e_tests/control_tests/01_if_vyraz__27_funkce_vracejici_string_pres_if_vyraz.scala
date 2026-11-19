import ifj26.*

def kind(n: Int): String =
  if n < 0 then
    "neg"
  else if n == 0 then
    "zero"
  else
    "pos"

@main
def main(): Unit =
  val a = kind(-3)
  val b = kind(0)
  val c = kind(3)
  print(a)
  print(b)
  print(c)
