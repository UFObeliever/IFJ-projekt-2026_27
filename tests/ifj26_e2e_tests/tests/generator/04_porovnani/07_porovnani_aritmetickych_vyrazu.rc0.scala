import ifj26.*

@main def main(): Unit =
  val a = 5
  val b = 10
  if a + 1 <= b - 3 then
    print("1")
  else
    print("0")
  if a * 2 > b then
    print("1")
  else
    print("0")
  if a * 2 >= b then
    print("1")
  else
    print("0")
  if a + b == 15 then
    print("1")
  else
    print("0")
  if b / a != 2 then
    print("1")
  else
    print("0")
  print("\n")
