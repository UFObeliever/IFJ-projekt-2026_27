import ifj26.*

def classify(x: Int): Unit =
  if x < 0 then
    print("negative")
  else if x == 0 then
    print("zero")
  else
    print("positive")

@main
def main(): Unit =
  classify(-5)
  classify(0)
  classify(7)
