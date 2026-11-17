import ifj26.*

def say(n: Int): Int =
  print(n)
  n

@main def main(): Unit =
  val a = say(1)
  if a > 5 then
    print("A")
  else if a > 3 then
    print("B")
  else if a > 0 then
    print("C")
  else
    print("D")
  print("\n")
