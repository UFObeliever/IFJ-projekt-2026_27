import ifj26.*

def isEven(n: Int): Boolean = n / 2 * 2 == n

@main
def main(): Unit =
  if isEven(4) then
    print("y")
  else
    print("n")
  if isEven(3) then
    print("y")
  else
    print("n")
