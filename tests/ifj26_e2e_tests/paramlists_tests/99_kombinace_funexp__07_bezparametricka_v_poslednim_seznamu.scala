import ifj26.*

def answer: Int = 42

def offset()(value: Int): Int = value + 1

@main
def main(): Unit =
  print(toStr(offset()(answer)))
