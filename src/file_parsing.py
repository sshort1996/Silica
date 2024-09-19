from openai import OpenAI

# Replace with your OpenAI API key
API_KEY = ""

client = OpenAI(api_key=API_KEY)
import sys
import json

def get_gpt_response(prompt, API_KEY):

    try:
        response = client.chat.completions.create(model="gpt-3.5-turbo",
        messages=[
            {"role": "user", "content": prompt}
        ])
        # Extract the response content
        message = response.choices[0].message.content
        print(message)
    except Exception as e:
        print(f"Error: {str(e)}", file=sys.stderr)

if __name__ == "__main__":
    # The prompt is passed as an argument from the C program
    if len(sys.argv) > 1:
        prompt = sys.argv[1]
        get_gpt_response(prompt, API_KEY)
    else:
        print("No prompt provided.", file=sys.stderr)

