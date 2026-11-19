import ifj26.*

def add(a: Int, b: Int): Int = a + b

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  if add(dbl(1), dbl(2)) == 6 then
    print("y")
  else
    print("n")
