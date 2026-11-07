import ifj26.*

def classify(n: Int): String =
  if n < 0 then
    return "negative"
  else
    if n == 0 then
      return "zero"
    else
      return "positive"

def sumTo(n: Int): Int =
  var s = 0
  var i = 1
  while i <= n do
    s = s + i
    i = i + 1
  return s

@main def main(): Unit =
  val a = readInt()
  val c = classify(a)
  print(c)
  print("\n")
  val s = sumTo(a)
  val t = toStr(s)
  print(t)
  print("\n")
