import ifj26.*

def answer: Int = 42

def f(x: Int): Int =
  return answer + x

@main
def main(): Unit =
  val r = f(1)
  print(r)
