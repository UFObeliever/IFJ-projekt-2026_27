import ifj26.*

def grade(g: Int): Unit =
  if g >= 90 then
    print("A")
  else if g >= 80 then
    print("B")
  else if g >= 70 then
    print("C")
  else if g >= 60 then
    print("D")
  else
    print("F")

@main
def main(): Unit =
  grade(95)
  grade(85)
  grade(75)
  grade(65)
  grade(10)
