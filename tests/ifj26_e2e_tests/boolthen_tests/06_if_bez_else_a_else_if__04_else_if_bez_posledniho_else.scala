import ifj26.*

def sign(x: Int): Unit =
  if x < 0 then
    print("negative")
  else if x == 0 then
    print("zero")

@main
def main(): Unit =
  sign(-1)
  sign(0)
  sign(5)
  print("|")
