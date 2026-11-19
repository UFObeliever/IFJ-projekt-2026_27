import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  if dbl(2) == 4 then
    print("a")
  else
    print("A")
  if dbl(2) != dbl(3) then
    print("b")
  else
    print("B")
  if dbl(3) > dbl(2) then
    print("c")
  else
    print("C")
  if dbl(2) <= dbl(2) then
    print("d")
  else
    print("D")
  if dbl(1) >= dbl(2) then
    print("E")
  else
    print("e")
  if dbl(1) < dbl(2) then
    print("f")
  else
    print("F")
