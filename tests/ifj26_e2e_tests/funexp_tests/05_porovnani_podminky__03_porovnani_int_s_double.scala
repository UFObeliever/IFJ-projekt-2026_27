import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  if dbl(2) < 4.5 then
    print("y")
  else
    print("n")
  if dbl(2) == 4.0 then
    print("y")
  else
    print("n")
