import ifj26.*

def answer: Int = 42

def add(a: Int, b: Int): Int = a + b

@main
def main(): Unit =
  val s = add(answer, answer)
  print(s)
