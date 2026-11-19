import ifj26.*

def answer: Int = 42

def wrap(): Int =
  print("w")
  answer

@main
def main(): Unit =
  val r = wrap()
  print(r)
